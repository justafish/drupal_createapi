<?php
/**
 * @file
 * Helper functions for creating JSON output.
 */

/**
 * Wrap an array of output with labels.
 *
 * @param array $rows
 *   The rows to wrap the value of $row_label around.
 * @param string $wrapper_label
 *   The value to wrap around the entire array.
 * @param string $row_label
 *   the value to wrap around each row.
 *
 * @return array
 *   An array with a key of $wrapper_label, containing an array of $row_labels
 *   containing each $row.
 */
function _createapi__helper__json_wrapper($rows, $wrapper_label, $row_label) {
  $output = array();
  foreach ($rows as $row) {
    $output[] = array($row_label => $row);
  }
  $output = array($wrapper_label => $output);

  return $output;
}

/**
 * Setup the response format for JSON.
 *
 * @param array $output
 *   An array of items to output as JSON.
 */
function _createapi__helper__json_response($output) {
  $json = drupal_json_encode($output);

  // Add JSONP callback.
  $query_params = drupal_get_query_parameters();
  $jsonp_callback = (isset($query_params['callback'])) ? $query_params['callback'] : FALSE;
  if ($jsonp_callback) {
    drupal_add_http_header('Content-Type', 'application/json; charset=utf-8');
    $json = "$jsonp_callback($json)";
  }
  else {
    drupal_add_http_header('Content-Type', 'text/javascript; charset=utf-8');
  }

  print $json;
}
