<?php

/**
 * Returns the results of a node_load() for the specified node.
 *
 * This returned node may optionally take content_permissions settings into
 * account, based on a configuration setting.
 *
 * @param $nid
 *   NID of the node we want to return.
 * @return
 *   Node object or FALSE if not found.
 *
 * @see node_load()
 */
function slg_json_category_retrieve($arg) {
  module_load_include('inc', 'slg', 'slg.details');
	if ($arg == 'index') {
	// Listing of all Categories and Subcategories
	  $terms = taxonomy_get_tree(1, 0, -1, 1);
	  $categories = array();
	  $output = array();
	  foreach ($terms as $term)
	  {
	  	$categories[] = $term;
	  }
	  for ($i = 0; $i < ceil(count($categories)); $i++) {
	  	$children = array();
	  	$terms = taxonomy_get_tree(1, $categories[$i]->tid, -1, 1);
	  	$temp_object = (object) array();
	  	foreach ($terms as $term) {
	  		$key = $term->name;
	  		$temp_object->$key = $term->tid;
	  	}
			$output[$categories[$i]->name] = $categories[$i]->tid;
	  }
	  return $output;
  }
  else {
	// List of Subcategory, tertiary categories, and companies.
	  $terms = taxonomy_get_tree(1, $arg, -1, 1);	
  	$temp_object = (object) array();
  	foreach ($terms as $term) {
  		$key = $term->name;
  		$temp_object->$key = $term->tid;
  	}
	  $output->categories = $temp_object;
		$tids[] = $arg;
		$sortDirection = 'asc';
		$query = "SELECT DISTINCT `n`.`nid` FROM {node} `n`, {term_node} `tn` WHERE `n`.`nid` = `tn`.`nid` AND `n`.`vid` = `tn`.`vid` AND `n`.`status` = 1 AND `n`.`type` = 'company' AND (|CATEGORIES|) ORDER BY `n`.`title` $sortDirection";
		$categories = array();
		$args = array();
		foreach ($tids as $t)
		{
			$categories[] = '`tn`.`tid` = %d';
			$args[] = $t;
		}
		$categories = implode(' OR ', $categories);
		$query = str_replace('|CATEGORIES|', $categories, $query);
		$result = pager_query($query, 100, 0, NULL, $args);
		$companies = array();
		while ($r = db_fetch_array($result))
		{
			$companies[] = node_load($r['nid']);
		}		
		foreach($companies as $company) {
			$key = $company->title;
			$company_object->$key->nid = $company->nid;
			$company_object->$key->teaser = _trim_string($company->field_description[0]['value']);
		}
		$output->companies = $company_object;
	  return $output;
  }
}

function slg_json_category_access() {
	return TRUE;
}

function slg_json_company_retrieve($nid) {
	global $user;
	$node = node_load($nid);
	
	if ($node) {
	  // Apply field level content permissions
	  if (module_exists('content') && variable_get('services_use_content_permissions', TRUE)) {
	    $fields = content_fields(NULL, $node->type);
	    foreach ($fields as $field_name => $field_info) {
	      if (isset($node->$field_name)) {
	        $access = module_invoke_all('field_access', 'view', $field_info, $user, $node);
	        if (in_array(FALSE, $access)) {
	          unset($node->$field_name);
	        }
	      }
	    }
	  }
	  $node->uri = services_resource_uri(array('node', $node->nid));
	  $node->body = $node->field_body[0]['value'];
		$node->teaser = _trim_string($node->field_description[0]['value']);
	  return $node;
	}
	return services_error('Node nid '. $nid .' not found', 404);
}

function slg_json_event_retrieve($nid) {
	global $user;
	if ($nid == 'index') {
		$node_ids = new stdClass;
		$node_return = new stdClass;
		$nodes = node_load_by_type('event');
		$to_sort = array();
		$titles = new stdClass;
		$time = new stdClass;
		foreach ($nodes as $node) {
			$attribute = $node->nid;
			$node_date = date('U' , strtotime($node->field_event_date[0]['value']));
      if( strtotime($node->field_event_date[0]['value']) >= date('U', time())) {
				$node_ids->$attribute = $node_date;
				$titles->$attribute = $node->title;
				$to_sort[$attribute] = $node_date;
				
				// Local Time
				$local_time = date('U' , strtotime($node->field_event_date[0]['value']));
				$date = $node->field_event_date[0]['value'];
				$local_zone = $node->field_event_date[0]['timezone'];
				$type = DATE_DATETIME;
				$date = date_make_date($date, 'UTC', $type);
				date_timezone_set($date, timezone_open($local_zone));
				$local_time = date_format_date($date, 'custom', 'H:i');
				$time->$attribute = $local_time;
				
			}
		}
		asort($to_sort);
		$count = 0;
		foreach ($to_sort as $key => $value) {
			$count++;
			if ($count <= 10) 
			  $node_return->$key = array(
					'date' => date('Y-m-d',$value),
					'title' => $titles->$key,
					'local_time' => $time->$key,
					);
		}
		return $node_return;
	}
	else {
	  $node = node_load($nid);
	  
	  if ($node) {
	    $node->uri = services_resource_uri(array('node', $node->nid));
	    $node->body = _trim_string($node->body);
	  	$node->teaser = _trim_string($node->field_description[0]['value']);
			$local_time = date('U' , strtotime($node->field_event_date[0]['value']));
			$date = $node->field_event_date[0]['value'];
			$local_zone = $node->field_event_date[0]['timezone'];
			$type = DATE_DATETIME;
			$date = date_make_date($date, 'UTC', $type);
			date_timezone_set($date, timezone_open($local_zone));
			$node->local_time = date_format_date($date, 'custom', 'H:i');
	    return $node;
	  }
	  return services_error('Node nid '. $nid .' not found', 404);
	}
}


function slg_json_company_access() {
	return TRUE;
}

function slg_json_jobs_retrieve($nid) {
	if ($nid == 'index') {
	  $view_most_recent = true;
	  $alpha_sort = 'desc';
	  $order = array('field' => 'title', 'direction' => 'asc');
	  $keywords = NULL;
	  if (array_key_exists('date', $_GET))
	  {
	  	$order = array('field' => 'date', 'direction' => 'desc');
	  	$view_most_recent = false;
	  }
	  else
	  {
	  	if (array_key_exists('alpha', $_GET))
	  	{
	  		$order = array('field' => 'title', 'direction' => 'desc');
	  		$alpha_sort = 'asc';
	  	}
	  }
	  $filter = array_key_exists('filter', $_GET) ? $_GET['filter'] : NULL;
	  if (!in_array($filter, valid_filters()))
	  {
	  	$filter = NULL;
	  }
	  $jobs = slg_get_jobs($filter, $order);
	  $type = 'listing';
	  if ($filter)
	  {
	  	$type = 'filter';
	  }
	  $result_count = count($jobs);
	  foreach($jobs as $job) {
	  	$key = $job->title;
	  	$result->$key->nid = $job->nid;
	  	$result->$key->teaser = _trim_string($job->teaser);
	  }
	  return $result;
  }
	else {
	  global $user;
	  $node = node_load($nid);
	  
	  if ($node) {
	    // Apply field level content permissions
	    if (module_exists('content') && variable_get('services_use_content_permissions', TRUE)) {
	      $fields = content_fields(NULL, $node->type);
	      foreach ($fields as $field_name => $field_info) {
	        if (isset($node->$field_name)) {
	          $access = module_invoke_all('field_access', 'view', $field_info, $user, $node);
	          if (in_array(FALSE, $access)) {
	            unset($node->$field_name);
	          }
	        }
	      }
	    }
	    $node->uri = services_resource_uri(array('node', $node->nid));
			$node->teaser = _trim_string($node->field_teaser[0]['value']);
	    return $node;
	  }
	  return services_error('Node nid '. $nid .' not found', 404);
	}
}

function slg_json_jobs_access() {
	return TRUE;
}


function slg_json_location_retrieve($radius, $cat, $lat, $long) {
	if ($cat == 99) {
		$lat_long_one = get_lat_long(urlencode('2518 Circle Drive Saint Louis MO, 63143'));
		$lat_long_two = get_lat_long(urlencode('6677 Delmar Blvd Saint Louis MO, 63130'));
		return distance($lat_long_one->lat, $lat_long_one->long, $lat_long_two->lat, $lat_long_two->long, 'm');
	}
	// Gather all companies from category
	if ($cat != null) {
		$cats = array();
		$comps = new stdClass();
	// List of Subcategory, tertiary categories, and companies.
		$objects = get_companies($cat);
		foreach ($objects->categories as $key => $value) {
			$companies = get_companies($value);
			foreach($companies->companies as $key_a => $value_b) {
				$cats[$key_a] = $value_b;
			}
			foreach ($companies->categories as $cat_key => $cat_value) {
				$more_companies = get_companies($cat_value);
				foreach($more_companies->companies as $a => $b) {
					$cats[$a] = $b;
				}
			}
		}
		foreach ($objects->companies as $key => $value) {
			$cats[$key] = $value;
		}
		foreach($cats as $name => $company) {
			$temp = new stdClass();
			$node = node_load($company->nid);
			$cats->$name->address = $node->field_mailing_address[0]->value;
			$temp->address = $node->field_mailing_address[0]['value'] . ' ' . $node->field_city[0]['value'] . ' ' . $node->field_state[0]['value'] . ' ' . $node->field_zip[0]['value'];
			$temp->nid = $node->nid;
			$temp->teaser = $company->teaser;
			$result = db_result(db_query('SELECT lat_long FROM {custom_latlong} WHERE nid = %d', $node->nid));
			if ($result == 'null' || $result == null) {
				$lat_long = get_lat_long(urlencode($temp->address));
				if ($lat_long != 'null' && $lat_long != null)
					db_query("INSERT INTO {custom_latlong} (nid, lat_long) VALUES ('%d', '%s')", $node->nid, json_encode($lat_long));
				$temp->lat_long = $lat_long;
			}
			else $temp->lat_long = json_decode($result);
			$temp->distance = distance($temp->lat_long->lat, $temp->lat_long->lng, $lat, $long, 'm');
			if ($temp->distance <= $radius)
				$comps->$name = $temp;
		}
		return $comps;
	}
}

function slg_json_location_access() {
	return TRUE;
}

function _trim_string($string) {
	$string = str_replace("\n", ' ', $string);
	$string = str_replace("\r", ' ', $string);
	$string = str_replace("\t", ' ', $string);
	$string = str_replace("&nbsp;", ' ', $string);
	$string = strip_tags($string);
	$string = trim($string);
	$string = preg_replace('/\s\s+/', ' ', $string);
	return $string;
}

function node_load_by_type($type, $limit = 10000, $offset = 0) {
  $nodes = array();
  $query = db_rewrite_sql("SELECT nid FROM {node} n WHERE type = '%s'", 'n');
  $results = db_query_range($query, $type, $offset, $limit);
  while($nid = db_result($results)) {
    $nodes[] = node_load($nid);
  }
  return $nodes;
}

function distance($lat1, $lon1, $lat2, $lon2, $unit) {
  $theta = $lon1 - $lon2;
  $dist = sin(deg2rad($lat1)) * sin(deg2rad($lat2)) +  cos(deg2rad($lat1)) * cos(deg2rad($lat2)) * cos(deg2rad($theta));
  $dist = acos($dist);
  $dist = rad2deg($dist);
  $miles = $dist * 60 * 1.1515;
  $unit = strtoupper($unit);

  if ($unit == "K") {
    return ($miles * 1.609344);
  } else if ($unit == "N") {
      return ($miles * 0.8684);
    } else {
        return $miles;
      }
}

function get_lat_long($address) {
	$res = drupal_http_request('http://maps.googleapis.com/maps/api/geocode/json?address=' . $address .'&sensor=false');
	return json_decode($res->data)->results[0]->geometry->location;
}

function get_companies($arg) {
	$terms = taxonomy_get_tree(1, $arg, -1, 1);	
  $temp_object = (object) array();
  foreach ($terms as $term) {
  	$key = $term->name;
  	$temp_object->$key = $term->tid;
  }
	$output->categories = $temp_object;
	$query = "SELECT DISTINCT `n`.`nid` FROM {node} `n`, {term_node} `tn` WHERE `n`.`nid` = `tn`.`nid` AND `n`.`vid` = `tn`.`vid` AND `n`.`status` = 1 AND `n`.`type` = 'company' AND (|CATEGORIES|) ORDER BY `n`.`title` $sortDirection";
	$categories = array();
	$args = array();
	foreach ($tids as $t)
	{
		$categories[] = '`tn`.`tid` = %d';
		$args[] = $t;
	}
	$categories = implode(' OR ', $categories);
	$query = str_replace('|CATEGORIES|', $categories, $query);
	$result = pager_query($query, 100, 0, NULL, $args);
	$companies = array();
	while ($r = db_fetch_array($result))
	{
		$companies[] = node_load($r['nid']);
	}		
	foreach($companies as $company) {
		$key = $company->title;
		$company_object->$key->nid = $company->nid;
		$company_object->$key->teaser = _trim_string($company->field_description[0]['value']);
	}
	$output->companies = $company_object;
	return $output;
}
