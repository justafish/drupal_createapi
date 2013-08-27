<?php

/**
 * @file
 *
 * Admin file for Create API.
 */

/**
 * Admin listing for CreateAPI Exposed Paths.
 */
function createapi_admin_listing() {
  // Lets create a table for listing.
  $header = array();
  $header[] = array(
    'data' => t('Path'),
    'field' => 'path',
    'sort' => 'asc',
  );
  $header[] = array(
    'data' => t('System'),
    'field' => 'system',
  );

  $rows = array();

  $content_type_endpoints = module_invoke_all('createapi_content_types');
  foreach ($content_type_endpoints as $content_type => $endpoint) {
    $path = 'api/' . $endpoint['version'] . '/' . $endpoint['path'];
    $row = array();
    $row['data']['path'] = l($path, $path);
    $row['data']['module'] = 'Content Type';
    $rows[] = $row;
  }

  $queue_endpoints = module_invoke_all('createapi_nodequeues');
  foreach ($queue_endpoints as $queue_name => $endpoint) {
    $path = 'api/' . $endpoint['version'] . '/' . $endpoint['path'];
    $row = array();
    $row['data']['path'] = l($path, $path);
    $row['data']['module'] = 'NodeQueues';
    $rows[] = $row;
  }

  $menu_endpoints = module_invoke_all('createapi_menus');
  foreach ($menu_endpoints as $menu_name => $endpoint) {
    $path = 'api/' . $endpoint['version'] . '/' . $endpoint['path'];
    $row = array();
    $row['data']['path'] = l($path, $path);
    $row['data']['module'] = 'Menus';
    $rows[] = $row;
  }

  $variable_endpoints = module_invoke_all('createapi_variables');
  foreach ($variable_endpoints as $endpoint_name => $endpoint) {
    $path = 'api/' . $endpoint['version'] . '/' . $endpoint['path'];
    $row = array();
    $row['data']['path'] = l($path, $path);
    $row['data']['module'] = 'Variable';
    $rows[] = $row;
  }

  $custom_endpoints= module_invoke_all('createapi_custom_entities_info');
  foreach ($custom_endpoints as $endpoint_name => $endpoint) {
    $path = 'api/' . $endpoint['version'] . '/' . $endpoint['path'];
    $row = array();
    $row['data']['path'] = l($path, $path);
    $row['data']['module'] = 'Entities';
    $rows[] = $row;
  }

  $build['path_table'] = array(
    '#theme' => 'table',
    '#header' => $header,
    '#rows' => $rows,
    '#empty' => t('No CreateAPI paths are available.'),
  );

  $build['path_pager'] = array('#theme' => 'pager');

  return $build;
  return $form;

  // Currently the module has no settings but leave this here for now.
  // return system_settings_form($form);
}
