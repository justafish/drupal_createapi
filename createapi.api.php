<?php

/**
 * @file
 * Hooks provided by the CreateAPI module.
 */

/**
 * Serve a JavaScript file by the getjs API by adding it into the page.
 *
 * @param string $api_version
 *   The API version.
 *
 * @return array
 *   An array of script paths that are whitelisted for delivery.
 */
function hook_createapi_addjs($api_version) {
  $js = array();

  if ($api_version === '1.0') {
    $js = array(
      'misc/collapse.js',
      'sites/all/modules/custom/pants/pants.js',
    );
  }

  return $js;
}

/**
 * Expose content types as endpoints.
 *
 * @return array
 *   Describes the properties of the endpoint, each element keyed by the machine
 *   name of each content type being exposed.
 */
function hook_createapi_content_types() {
  return array(
    // The content type machine name.
    'topic' => array(
      // This will create a path of the form "/api/<version>/<path>".
      'version' => '1.0',
      'path' => 'topics.json',
      // Wrapper fields for JSON data.
      'wrapper' => 'topics',
      'row' => 'topic',
      // Which fields and properties to be output, as well as the path.
      // The key represents the output alias and the value is the field or
      // property machine name.
      'data' => array(
        'fields' => array(
          'short_title' => 'field_short_title',
        ),
        'properties' => array(
          'id' => 'nid',
          'title' => 'title',
          'created' => 'created',
        ),
        // The value is used as the output alias.
        'path' => 'path',
      ),
      // URL filters that can be used with this endpoint.
      'filters' => array(
        // The key represents the parameter to be used in the URL and the value
        // is the property to filter by.
        'properties' => array(
          'id' => 'nid',
        ),
        // The value is used as the URL parameter.
        'path' => 'path',
        'start_end' => array(
          // The property to restrict results by. This could also be
          // 'field' => 'field_name', however the property will override the
          // field if both are present.
          'property' => 'created',
          // The values are used in the URL parameters.
          'start' => 'start',
          'end' => 'end',
        ),
        // The values are used in the URL parameters. There is an enforced limit
        // of 200 items for the range.
        'range' => 'count',
        'offset' => 'offset',
      ),
    ),
  );
}

/**
 * Expose nodequeues as endpoints.
 *
 * @return array
 *   Describes the properties of the endpoint, each element keyed by the machine
 *   name of each nodequeue being exposed.
 *
 * @see hook_createapi_content_types()
 */
function hook_createapi_nodequeues() {
  return array(
    // The nodequeue machine name.
    'trending_topics' => array(
      'version' => '1.0',
      'path' => 'trending-topics.json',
      'wrapper' => 'topics',
      'row' => 'topic',
      'data' => array(
        'fields' => array(
          'short_title' => 'field_short_title',
        ),
        'properties' => array(
          'id' => 'nid',
          'title' => 'title',
        ),
        'path' => 'path',
      ),
      // Only the following filters are supported in nodequeue endpoints.
      'filters' => array(
        'properties' => array(
          'id' => 'nid',
        ),
        'path' => 'path',
        'range' => 'count',
        'offset' => 'offset',
      ),
    ),
  );
}

/**
 * Expose any entities as an endpoint.
 *
 * @return array
 *   Describes the properties of the endpoint.
 *
 * @see hook_createapi_content_types()
 */
function hook_createapi_custom_entities_info() {
  return array(
    'topics' => array(
      'version' => '1.0',
      'path' => 'topics.json',
      'wrapper' => 'topics',
      'row' => 'topic',
      'data' => array(
        'fields' => array(
          'short_title' => 'field_short_title',
        ),
        'properties' => array(
          'id' => 'nid',
          'title' => 'title',
          'created' => 'created',
        ),
        'path' => 'path',
      ),
      'filters' => array(
        'properties' => array(
          'id' => 'nid',
        ),
        'path' => 'path',
        'start_end' => array(
          'property' => 'created',
          'start' => 'start',
          'end' => 'end',
        ),
        'range' => 'count',
        'offset' => 'offset',
      ),
      'custom_query' => array(
        'entity_type' => 'node',
        // This item is only required if path filtering is used. It is the alias
        // of the node ID as used in the EntityFieldQuery.
        'nid_alias' => 'nid',
      ),
    ),
  );
}

/**
 * The query that is used for fetching the set of entities to use for custom
 * endpoints.
 *
 * @return EntityFieldQuery
 */
function hook_createapi_custom_entities_query_ENDPOINT_ID() {
  $query = new EntityFieldQuery();
  $query->entityCondition('entity_type', 'node')
    ->entityCondition('bundle', 'topic')
    ->propertyCondition('status', 1)
    ->propertyOrderBy('nid', 'DESC');

  return $query;
}

