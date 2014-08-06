<?php
/**
 * @file
 * Helper functions used to create feeds for CreateAPI.
 */

include 'createapi.helpers.filters.inc';
include 'createapi.helpers.json.inc';
/**
 * Process a set of entities IDs, of one type, into an array for output.
 *
 * @param array $eids
 *   An array of IDs.
 * @param string $entity_type
 *   The entity type being dealt with.
 * @param array $fields
 *   (optional) Fields to be output. @see hook_createapi_custom_entities().
 * @param array $properties
 *   (optional) Properties to be output. @see hook_createapi_custom_entities().
 * @param NULL|string $path
 *   (optional) The key to use, if any, when outputting the path of this entity.
 *
 * @return array
 *   A multidimensional array that can be easily passed into a function such as
 *  json_encode().
 */
function _createapi__helper__process_entities(array $eids, $entity_type, $fields = array(), $properties = array(), $path = FALSE) {
  $output = array();
  $entities = entity_load($entity_type, $eids);

  foreach ($entities as $entity) {
    $field_output = array();

    foreach ($properties as $key => $property) {
      if (isset($entity->{$property})) {
        $field_output[$key] = $entity->{$property};
      }
    }

    foreach ($fields as $key => $field) {
      $field_output[$key] = _createapi__helper__feed_format_field($entity_type, $entity, $field);
    }

    if (isset($path) && $path) {
      $path_info = entity_uri($entity_type, $entity);
      if (isset($path_info['path'])) {
        $field_output[$path] = $path_info['path'];
        $aliased_path = drupal_lookup_path('alias', $path_info['path']);
        if ($aliased_path) {
          $field_output[$path] = drupal_strtolower($aliased_path);
        }
      }
    }

    $output[] = $field_output;
  }

  return $output;
}

/**
 * Format different field types into an output suitable for use in a feed.
 *
 * @param string $entity_type
 *   The type of entity that is being acted upon e.g. 'node' or 'file'.
 * @param stdClass $entity
 *   The entity associated with the field to be formatted.
 * @param string $field_name
 *   The name of the field to be formatted.
 *
 * @return mixed
 *   The formatted field as rendered using field_view_value(). The result will
 *   either be a string or array depending on the cardinality of the field.
 */
function _createapi__helper__feed_format_field($entity_type, $entity, $field_name) {
  $output = array();
  // File entities or entity references may have attached fields.
  if (is_array($field_name)) {
    $current = current($field_name);
    $field_name = key($field_name);
  }
  if (isset($entity->{$field_name})) {
    $field_info = field_info_field($field_name);
    $items = field_get_items($entity_type, $entity, $field_name);

    if (is_array($items)) {
      for ($i = 0; $i < count($items); $i++) {
        if (isset($items[$i])) {
          switch ($field_info['type']) {
            case 'text':
            case 'text_long':
            case 'text_with_summary':
            case 'list_text':
              $view = field_view_value($entity_type, $entity, $field_name, $items[$i]);
              if (isset($view['#markup'])) {
                $output[] = $view['#markup'];
              }
              break;

            case 'entityreference':
              $entity_type = $field_info['settings']['target_type'];
              $view = field_view_value($entity_type, $entity, $field_name, $items[$i]);
              $reference = array();

              if (isset($items[$i]['target_id'])) {
                $target_id = $items[$i]['target_id'];
                $reference = array(
                  'eid' => $target_id ,
                );

                if (isset($current)) {
                  $referenced_entity = entity_load($entity_type, array($target_id));
                  if (isset($referenced_entity[$target_id])) {
                    $referenced_entity = $referenced_entity[$target_id];
                    // Fields.
                    if (isset($current['fields'])) {
                      foreach ($current['fields'] as $key => $field) {
                        $reference[$key] = _createapi__helper__feed_format_field($entity_type, $referenced_entity, $field);
                      }
                    }
                    // Properties.
                    if (isset($current['properties'])) {
                      foreach ($current['properties'] as $key => $property) {
                        $reference[$key] = $referenced_entity->{$property};
                      }
                    }
                    // Path.
                    if (isset($current['path'])) {
                      $path = entity_uri($entity_type, $referenced_entity);
                      $reference[$current['path']] = drupal_get_path_alias($path['path']);
                    }
                  }
                }
                elseif (isset($view['#markup'])) {
                  $reference['value'] = $view['#markup'];
                }
              }
              $output[] = $reference;
              break;

            case 'field_collection':
              $entity_id = $items[$i]['value'];
              $referenced_entity = entity_load('field_collection_item', array($entity_id));
              $reference = array();
              if ($referenced_entity) {
                $reference = array(
                  'eid' => $entity_id ,
                );
                if (isset($current)) {
                  $referenced_entity = $referenced_entity[$entity_id];
                  // Fields.
                  if (isset($current['fields'])) {
                    foreach ($current['fields'] as $key => $field) {
                      $reference[$key] = _createapi__helper__feed_format_field('field_collection_item', $referenced_entity, $field);
                    }
                  }
                  // Properties.
                  if (isset($current['properties'])) {
                    foreach ($current['properties'] as $key => $property) {
                      $reference[$key] = $referenced_entity->{$property};
                    }
                  }
                }
              }
              $output[] = $reference;
              break;

            case 'datetime':
              $output[] = strtotime($items[$i]['value']);
              break;
            case 'datestamp':
            case 'time_of_day':
              $output[] = $items[$i]['value'];
              break;

            case 'image':
              $image = field_view_value($entity_type, $entity, $field_name, $items[$i], array(
                'type' => 'image',
                'settings' => array(
                  'image_link' => 'file',
                ),
              ));

              // Drupal gives us the file entity as an array, so convert this to
              // an object.
              $file = new stdClass();
              foreach ($image['#item'] as $key => $value) {
                $file->{$key} = $value;
              }

              $file_output = array(
                'path' => $image['#path']['path'],
                'width' => $image['#item']['width'],
                'height' => $image['#item']['height'],
                'mime' => $image['#item']['filemime'],
              );

              // Format the output for fields that are attached to this file
              // entity by recursively calling this function on child fields.
              if (!empty($current['fields'])) {
                foreach ($current['fields'] as $key => $field) {
                  $file_output[$key] = _createapi__helper__feed_format_field('file', $file, $field);
                }
              }

              // Add image styles.
              if (!empty($current['styles'])) {
                foreach ($current['styles'] as $key => $style) {
                  $file_output['styles'][$key] = image_style_url($style, $image['#item']['uri']);
                }
              }

              $output[] = $file_output;
              break;

            case 'link_field':
              $output[] = array(
                'url' => (isset($items[$i]['url'])) ? $items[$i]['url'] : '',
                'title' => (isset($items[$i]['title'])) ? $items[$i]['title'] : '',
              );
              break;
            case 'list_boolean':
            case 'list_integer':
              $output[] = (int) $items[$i]['value'];
              break;
          }
        }
      }
    }
    // If the field only takes one value, make sure the output isn't an array.
    if (count($output) === 1 && $field_info['cardinality'] == 1) {
      $output = array_pop($output);
    }
  }

  return $output;
}

/**
 * Retrieve nodes in a given nodequeue.
 *
 * @param string $queue_name
 *   The machine name of the nodequeue.
 * @param array $fields
 *   (optional) Fields to be output. @see hook_createapi_nodequeues).
 * @param array $properties
 *   (optional) Properties to be output. @see hook_createapi_nodequeues().
 * @param NULL|string $path
 *   (optional) The key to use, if any, when outputting the path of this entity.
 * @param array $filters
 *   (optional) Which URL filters can be applied to this endpoint. @see
 *   hook_createapi_nodequeues().
 *
 * @return array
 *   A formatted array of nodes for outputting.
 */
function _createapi__helper__process_node_queue($queue_name, $fields = array(), $properties = array(), $path = FALSE, $filters = array()) {
  $nids = array();

  $query = db_select('nodequeue_queue', 'q');
  $query->join('nodequeue_nodes', 'o', 'o.qid = q.qid AND q.name = :qname', array(':qname' => $queue_name));
  $query->join('node', 'n', 'o.nid = n.nid');
  $query
    ->fields('n', array('nid'))
    ->condition('n.status', 1)
    ->orderBy('position', 'ASC');

  // Limit results.
  $range = (isset($filters['range'])) ? $filters['range']: FALSE;
  $offset = (isset($filters['offset'])) ? $filters['offset']: FALSE;
  _createapi__helper__filter__range_offset($query, $range, $offset);

  foreach ($filters as $filter => $items) {
    if (!$query) {
      break;
    }
    switch ($filter) {
      case 'properties':
        foreach ($items as $param => $property) {
          _createapi__helper__filter__property($query, $param, 'n.' . $property);
        }
        break;

      case 'path':
        _createapi__helper__filter__node_path($query, $items, 'n.nid');
        break;

    }
  }

  if ($query) {
    $result = $query->execute();
    foreach ($result as $record) {
      $nids[] = $record->nid;
    }
  }
  return _createapi__helper__process_entities($nids, 'node', $fields, $properties, $path);
}

/**
 * Retrieves items in a given menu.
 *
 * @param string $menu_name
 *   The machine name of the menu.
 *
 * @return array
 *   A formatted array of menu items for outputting.
 */
function _createapi__helper__process_menu($menu_name) {
  $links = menu_load_links($menu_name);
  $formatted_links = array();
  foreach ($links as $link) {
    if (!$link['hidden']) {
      $nid = '';
      if (substr($link['link_path'], 0, 5) === 'node/') {
        $nid = substr($link['link_path'], 5);
      }
      $formatted_links[] = array(
        'internal_path' => $link['link_path'],
        'alias' => drupal_get_path_alias($link['link_path']),
        'title' => $link['link_title'],
        'nid' => $nid,
      );
    }
  }
  return $formatted_links;
}

/**
 * Retrieves variables.
 *
 * @param array
 *   Variables to return. Keys will be the new array keys and the values are the
 *   variable names as seen in the variable table.
 *
 * @return array
 *   A formatted array of variables for outputting.
 */
function _createapi__helper__process_variables($variables) {
  foreach ($variables as $key => $value) {
    $variables[$key] = variable_get($value);
  }

  return $variables;
}

/**
 * Retrieve nodes of a given content type.
 *
 * @param string $content_type
 *   The node content type.
 * @param array $fields
 *   (optional) Fields to be output. @see hook_createapi_content_types().
 * @param array $properties
 *   (optional) Properties to be output. @see hook_createapi_content_types().
 * @param NULL|String $path
 *   (optional) The key to use, if any, when outputting the path of this entity.
 * @param array|bool $filters
 *   (optional) Which URL filters can be applied to this endpoint. @see
 *   hook_createapi_content_types().
 *
 * @return array
 *   A formatted array of nodes for outputting.
 */
function _createapi__helper__process_content_type($content_type, $fields = array(), $properties = array(), $path = FALSE, $filters = array()) {
  $query = new EntityFieldQuery();
  $query->entityCondition('entity_type', 'node')
    ->entityCondition('bundle', $content_type)
    ->propertyCondition('status', 1)
    ->propertyOrderBy('nid', 'DESC');

  return _createapi__helper__process_custom($query, 'node', $fields, $properties, $path, $filters, 'nid');
}

/**
 * Retrieve entities from a given EntityFieldQuery.
 *
 * @param EntityFieldQuery $query
 *   The un-executed EntityFieldQuery that will give us a set of entity IDs.
 * @param string $entity_type
 *   The type of entity being queried.
 * @param array $fields
 *   (optional) Fields to be output. @see hook_createapi_custom_entities().
 * @param array $properties
 *   (optional) Properties to be output. @see hook_createapi_customentities().
 * @param NULL|String $path
 *   (optional) The key to use, if any, when outputting the path of this entity.
 * @param array|bool $filters
 *   (optional) Which URL filters can be applied to this endpoint. @see
 *   hook_createapi_custom_entities().
 * @param bool|string $nid_alias
 *   (optional) If this is a node entity, provide the alias used in the query
 *   for the nid in order to allow path filtering to function.
 *
 * @return array
 *   A formatted array of entities for outputting.
 */
function _createapi__helper__process_custom(EntityFieldQuery $query, $entity_type, $fields = array(), $properties = array(), $path = FALSE, $filters = array(), $nid_alias = FALSE) {
  $eids = array();

  // Add filters.
  // Limit results.
  $range  = FALSE;
  $offset = FALSE;
  _createapi__helper__filter__range_offset($query, $range, $offset);

  foreach ($filters as $filter => $items) {
    if (!$query) {
      break;
    }
    switch ($filter) {
      case 'properties':
        foreach ($items as $param => $property) {
          _createapi__helper__filter__property($query, $param, $property);
        }
        break;

      case 'fields':
        foreach ($items as $param => $field) {
          _createapi__helper__filter__field($query, $param, $field['column'], $field['field']);
        }
        break;

      case 'path':
        if ($entity_type === 'node' && $nid_alias) {
          _createapi__helper__filter__node_path($query, $items, $nid_alias);
        }
        break;

      // If we set a range/offset on a query, it gets overridden by a further
      // call, so we can safely call it twice.
      case 'range':
        $range = $items;
        _createapi__helper__filter__range_offset($query, $range, $offset);
        break;

      case 'offset':
        $offset = $items;
        _createapi__helper__filter__range_offset($query, $range, $offset);
        break;

      case 'start_end':
        $property = (isset($items['property'])) ? $items['property'] : FALSE;
        $field = (isset($items['field'])) ? $items['field'] : FALSE;
        _createapi__helper__filter__start_end($query, $items['start'], $items['end'], $property, $field);
        break;
    }
  }

  if ($query) {
    $result = $query->execute();
    if (isset($result['node'])) {
      $eids = array_keys($result['node']);
    }
  }

  return _createapi__helper__process_entities($eids, $entity_type, $fields, $properties, $path);
}
