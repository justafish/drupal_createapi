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
        $aliased_path = drupal_lookup_path('alias', $path_info['path']);
        if ($aliased_path) {
          $field_output[$path] = $aliased_path;
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
  // File entities may have attached fields.
  if (is_array($field_name)) {
    $current = current($field_name);
    $attached_fields = (isset($current['fields'])) ? $current['fields'] : array();
    $styles = (isset($current['styles'])) ? $current['styles'] : array();
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
              $view = field_view_value($entity_type, $entity, $field_name, $items[$i]);
              if (isset($items[$i]['target_id']) && isset($view['#markup'])) {
                $output[] = array(
                  'eid' => $items[$i]['target_id'],
                  'value' => $view['#markup'],
                );
              }
              break;

            case 'datestamp':
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
              if (!empty($attached_fields)) {
                $children = array();
                foreach ($attached_fields as $key => $field) {
                  $children[$key] = _createapi__helper__feed_format_field('file', $file, $field);
                }
                $file_output = array_merge($children, $file_output);
              }

              // Add image styles.
              if (!empty($styles)) {
                foreach ($styles as $key => $style) {
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
  $range  = FALSE;
  $offset = FALSE;
  _createapi__helper__filter__range_offset($query, $range, $offset);

  foreach ($filters as $filter => $items) {
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
