<?php
/**
 * @file
 * Database filters that are activated by URL parameters.
 */

/**
 * Adds a filter by property to a database query from a URL parameter.
 *
 * @param SelectQuery|EntityFieldQuery $query
 *   The query being acted upon.
 * @param string $param
 *   The URL parameter to obtain the property value from.
 * @param string $property
 *   The property to filter by.
 */
function _createapi__helper__filter__property(&$query, $param, $property) {
  $query_params = drupal_get_query_parameters();
  $param_value = (isset($query_params[$param])) ? $query_params[$param] : FALSE;

  if ($param_value) {
    $param_value_split = explode(',', $param_value);
    $type = get_class($query);
    switch ($type) {
      case 'SelectQuery':
        $query->condition($property, $param_value_split);
        break;

      case 'EntityFieldQuery':
        $query->propertyCondition($property, $param_value_split);
        break;
    }
  }
}

/**
 * Adds a filter by field value to a database query from a URL parameter.
 *
 * @param EntityFieldQuery $query
 *   The query being acted upon.
 * @param $param
 *   The URL parameter to obtain the field value from.
 * @param $field
 *   The field to filter by.
 */
function _createapi__helper__filter__field(EntityFieldQuery &$query, $param, $column, $field) {
  $query_params = drupal_get_query_parameters();
  $param_value = (isset($query_params[$param])) ? $query_params[$param] : FALSE;

  if ($param_value) {
    $param_value_split = explode(',', $param_value);
    $query->fieldCondition($field, $column, $param_value_split, 'IN');
  }
}

/**
 * Adds a filter by node path to a database query from a URL parameter.
 *
 * @param SelectQuery|EntityFieldQuery $query
 *   The query being acted upon.
 * @param string $param
 *   The URL parameter to obtain the node path from.
 * @param string $nid_field
 *   (optional) The alias of the nid field in $query.
 */
function _createapi__helper__filter__node_path(&$query, $param, $nid_field = 'nid') {
  $query_params = drupal_get_query_parameters();
  $path = (isset($query_params[$param])) ? $query_params[$param] : FALSE;
  $type = get_class($query);

  if ($path) {
    $path_split = explode(',', $path);
    $nids = array();
    foreach ($path_split as $path) {
      $internal_path = drupal_get_normal_path($path);
      if (strlen($internal_path) > 5 && substr($internal_path, 0, 5) === 'node/') {
        $nid = substr($internal_path, 5);
        if (is_numeric($nid)) {
          $nids[] = $nid;
        }
      }
    }
    if (!empty($nids)) {
      switch ($type) {
        case 'SelectQuery':
          $query->condition($nid_field, $nids);
         break;

        case 'EntityFieldQuery':
         $query->propertyCondition($nid_field, $nids);
         break;
      }
    }
    else {
      $query = NULL;
    }
  }
}

/**
 * Adds a filter by offset to a database query from the URL parameter.
 *
 * @param SelectQuery|EntityFieldQuery $query
 *   The query being acted upon.
 * @param bool|int $param_range
 *   (optional) The number of results to return. There is a hard limit of 200.
 * @param bool|int $param_offset
 *   (optional) The number of records to offset the result by. e.g. A value of
 *   "1" here would omit the first result.
 */
function _createapi__helper__filter__range_offset(&$query, $param_range = FALSE, $param_offset = FALSE) {
  $query_params = drupal_get_query_parameters();
  $range = ($param_range && isset($query_params[$param_range]) && is_numeric($query_params[$param_range]) && $query_params[$param_range] <= 200) ? $query_params[$param_range] : 200;
  $offset = ($param_offset && isset($query_params[$param_offset]) && is_numeric($query_params[$param_offset])) ? $query_params[$param_offset] : 0;

  if ($range > 0) {
    $type = get_class($query);

    switch ($type) {
      case 'SelectQuery':
        $query->range($offset, $range);
        break;

      case 'EntityFieldQuery':
        $query->range($offset, $range);
        break;
    }
  }
  else {
    $query = NULL;
  }
}

/**
 * Adds filters for start and end ranges to a database query.
 *
 * This is typically used to retrieve results between date ranges.
 *
 * @param EntityFieldQuery $query
 *   The query being acted upon.
 * @param bool|int $param_start
 *   (optional) The value to start from.
 * @param bool|int $param_end
 *   (optional) The value to end from.
 * @param bool|string $property
 *   (optional) Which property to filter by.
 * @param bool|string $field
 *   (optional) Which field to filter by. This is not used if the property value
 *   is not
 *   FALSE.
 */
function _createapi__helper__filter__start_end(EntityFieldQuery &$query, $param_start = FALSE, $param_end = FALSE, $property = FALSE, $field = FALSE) {
  $query_params = drupal_get_query_parameters();
  $start = ($param_start && isset($query_params[$param_start]) && is_numeric($query_params[$param_start])) ? $query_params[$param_start] : FALSE;
  $end = ($param_end && isset($query_params[$param_end]) && is_numeric($query_params[$param_end])) ? $query_params[$param_end] : FALSE;
  if ($property) {
    if ($start) {
      $query->propertyCondition($property, $start, '>=');
    }
    if ($end) {
      $query->propertyCondition($property, $end, '<=');
    }
  }
  elseif ($field) {
    if ($start) {
      $query->fieldCondition($field, 'value', $start, '>=');
    }
    if ($end) {
      $query->fieldCondition($field, 'value', $end, '<=');
    }
  }
  else {
    $query = FALSE;
  }
}
