<?php

/*
|--------------------------------------------------------------------------
| Application Routes
|--------------------------------------------------------------------------
|
| Here is where you can register all of the routes for an application.
| It's a breeze. Simply tell Laravel the URIs it should respond to
| and give it the Closure to execute when that URI is requested.
|
*/
Route::get('/','HomeController@angular');
Route::get('/recovery_password/{user}/{token}','HomeController@passwordRecovery');

Route::group(array('prefix'=>'api'),function(){
  //user routes
  Route::post('user/login','UserController@login');
  Route::get('user/logout','UserController@logout');
  Route::get('user','UserController@index');
  Route::post('user/create_user','UserController@createUser');    
  Route::get('user/get_permitted_devices','UserController@getPermittedDevices');
  Route::get('user/{user}','UserController@displayUser');
  Route::put('user/{user}','UserController@editUser');    
  Route::delete('user/{user}','UserController@deleteUser');
  Route::get('user/{user}/get_folders','UserController@getFolders');
  Route::post('user/{user}/send_recovery','UserController@passwordRecovery');
  Route::put('user/{user}/password_recovery','UserController@setPassword');

  
  
  //folder routes
  Route::post('folder/create_folder','FolderController@createFolder');
  Route::delete('folder/{folder}','FolderController@deleteFolder');
  Route::post('folder/{folder}','FolderController@editFolder');
  Route::get('folder/{folder}/get_children','FolderController@getChildren');
  Route::put('folder/{folder}/set_parent','FolderController@setParent');
  Route::put('folder/{folder}/add_device','FolderController@addDevice'); 
  Route::get('folder/{folder}/get_devices','FolderController@getDevices');
  Route::delete('folder/{folder}/{device}','FolderController@removeDevice');

  //device routes
  Route::get('device/{name}','DeviceController@view');
  Route::post('device/{name}','DeviceController@edit');
  Route::get('device/{name}/get_favorites','DeviceController@getFavorites');
  Route::get('device/{name}/get_transducers','DeviceController@getTransducers');
  Route::get('device/{name}/get_permitted_users','DeviceController@getPermittedUsers');
  Route::get('device/{name}/get_storage_urls','DeviceController@getStorageUrls');
  Route::get('device/{id}/automap','DeviceController@getAutomap');
  Route::get('device/{name}/get_transducers_last_value','DeviceController@getTransducersLastValue');
  Route::post('device/{id}/subscribe','DeviceController@subscribeUser');
  Route::post('device/{id}/unsubscribe','DeviceController@unSubscribeUser');
  // Route::post('device/{id}/automap/{copyid}','DeviceController@copyLocation');
  Route::post('device/{name}/{transducer}/run_command','DeviceController@runCommand');
  Route::post('device/{name}/add/{user}','DeviceController@addDevicePermission');
  Route::delete('device/{name}/remove/{user}','DeviceController@removeDevicePermission');
  Route::get('device/{name}/{transducer}/{start}/{end}','DeviceController@getTransducerTimeseries');

  //schedule routes
  Route::get('schedule/{device}','ScheduleController@index');
  Route::post('schedule/{device}','ScheduleController@create');
  Route::put('schedule/{device}/{schedule}','ScheduleController@edit');
  Route::delete('schedule/{device}/{schedule}','ScheduleController@delete');
  Route::post('schedule/{device}/{schedule}/exceptions','ScheduleController@setExceptions');
  
  // monitor routes
  Route::get('monitor','MonitorController@index');
  Route::post('monitor','MonitorController@create');
  Route::get('monitor/{monitor}','MonitorController@view');
  Route::put('monitor/{monitor}','MonitorController@edit');
  Route::delete('monitor/{monitor}','MonitorController@delete');
});
