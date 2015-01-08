<?php

class DeviceController extends BaseController {
  
  /**
   * Get transducers from a device
   * @param  string $strDeviceId 
   * @return  json with a bolean variable and mixed, device transducers on success
   */
  public function getTransducers($strDeviceId){
    try{
      $device = Device::getById($strDeviceId,$this->user['username'],$this->user['password']);
    }catch(Exception $e){
      return Response::json(array('error'=>true,'message'=>$e->getMessage()));
    }
    return Response::json(array('error'=>false,'transducers'=>$device->transducers));
  }
  /**
   * Given a device and a command, will execute it.
   * @param  string $strDeviceId device id
   * @return JSON error array or success
   */
  public function runCommand($strDeviceId,$strTransducer){
    $intValue = Input::get('value',null);
    if(is_null($intValue)){
      return Response::json(array('error'=>true,'message'=>'Missing value'));
    }
    $transducer=new Transducer;
    $transducer->device = $strDeviceId;
    $transducer->name = $strTransducer;
    $response = $transducer->actuate($intValue,$this->user['username'],$this->user['password']);
    return Response::json($response);
  }
  /**
   * Subscribe a user to a device
   * @param $strDeviceId device id
   * @return json with a error bool variable and text message
   */
  public function subscribeUser($strDeviceId){
    //init device 
    $device = new Device;
    $device->id = $strDeviceId;
    $response = $device->subscribeUser($this->user['username'],$this->user['password']);
    return Response::json($response);
  }
  /**
   * Unsubscribe a user to a device
   * @param $strDeviceId device id
   * @return json with a error bool variable and text message
   */
  public function unSubscribeUser($strDeviceId){
    //init device 
    $device = new Device;
    $device->id = $strDeviceId;
    $response = $device->unSubscribeUser($this->user['username'],$this->user['password']);
    return Response::json($response);
  }
  /**
   * Allows editing of a device
   * @param  string $strDeviceId device name
   * @return JSON array with error, on success
   */
  public function view($strDeviceId){
    try{
      $device = Device::getById($strDeviceId,$this->user['username'],$this->user['password']);
    }catch(Exception $e){
      return Response::json(array('error'=>true,'message'=>$e->getMessage()));
    }
    return Response::json(array('error'=>false,'device'=>$device));
  }
  /**
   * Allows editing of a device
   * @param  string $strDeviceId device name
   * @return JSON array with error, on success
   */
  public function edit($strDeviceId){
    $strInfo = Input::get('info',null);
    $strLon = Input::get('lon',null);
    $strLat = Input::get('lat',null);
    $strStreet = Input::get('street',null);
    $strBuilding = Input::get('building',null);
    $strFloor = Input::get('floor',null);
    $strRoom = Input::get('room',null);
    $strOldParent = Input::get('old_parent', null);
    $strNewParent = Input::get('new_parent', null);
    $strDeviceName = Input::get('name',null);
    $strImageUrl = Input::get('image_url');

    if(!is_null($strOldParent) && !is_null($strNewParent) && $strOldParent != $strNewParent){
      $folder=new Folder;
      $folder->id=$strDeviceId;
      $response = $folder->setParent($strOldParent,$strNewParent,$this->user['username'],$this->user['password']);
      if($response['error']){
        return Response::json(array('error'=>true,'message'=>'Could not set the parent'));
      }
    }
    $device = new Device;
    $device->id = $strDeviceId;
    $device->name = $strDeviceName;
    $device->info = $strInfo;
    if(isset($strImageUrl) && !empty($strImageUrl)){
      $device->image = $strImageUrl;
    }else if(Input::hasFile('device_image')){
      $device->image = $this->uploadFile('device_image');
    }
    if(!is_null($strLon) && !is_null($strLat)){
      $device->lon=$strLon;
      $device->lat=$strLat;
      $device->street = $strStreet;
      $device->building = $strBuilding;
      $device->floor = $strFloor;
      $device->room = $strRoom;
    }
    
    $response = $device->edit($this->user['username'],$this->user['password']);

    return Response::json($response);
  }
  /**
   * Upload a file
   * @param  strinf $strFile       Input file identifier
   * @return string or array with error boolean and message
   */
  private function uploadFile($strFile){  
    $file= Input::file($strFile);
    $fileName = UUID::generate(4)->string.'.'.$file->getClientOriginalExtension();
    if($file->move(public_path().'/img/devices/',$fileName) ){
      return '/img/devices/'.$fileName;
    }
    return null;
  }
  /**
   * Gets historical data of a device
   * @param  string $strDeviceId device from which retrieve data
   * @param  string $strTransducer transducer identifier
   * @param  string $strStart      start date
   * @param  string $strEnd        end date
   * @return json containing historical data of device
   */
  public function getTransducerTimeseries($strDeviceId,$strTransducer,$strStart,$strEnd){
    $transducer = new Transducer;
    $transducer->device = $strDeviceId;
    $transducer->name = $strTransducer;
    $response = $transducer->getTimeseriesData($strStart,$strEnd,$this->user['username'],$this->user['password']);
    return Response::json($response);
  }
  /**
   * Get all favorites folders of a device
   * @param  string $strDeviceId name of the device
   * @return json array with error boolean variable and mixed text message or object 
   */
  public function getFavorites($strDeviceId){
    $device = new Device;
    $device->id = $strDeviceId;
    $response = $device->getFavorites($this->user['username'],$this->user['password']);
    return Response::json($response);
  }

  /**
   * Get all users of a device
   * @param  string $strDeviceId name of the device
   * @return json array with error boolean variable and mixed text message or object 
   * @todo get permitted devices
   */
  public function getPermittedUsers($strDeviceId){
    $device = new Device;
    $device->id = $strDeviceId;
    $response = $device->getPermittedUsers($this->user['username'],$this->user['password']);
    return Response::json($response);
  }
  /**
   * Grants device permission to a user
   * @param string $strUsername user to grant permission (subscribe)
   * @param int $strDeviceId device to grant permission to
   * @return json containig a message of either success or error
   */
  public function addDevicePermission($strDeviceId,$strUsername){
    $device = new Device;
    $device->id = $strDeviceId;
    $response = $device->addDevicePermission($strUsername,$this->user['username'],$this->user['password']);
    return Response::json($response);
  }
  /**
   * Get a list of device with same data as giving device
   * @param string $strDeviceId device id
   * @return json containig a message of either success or error 
   */
  public function getAutomap($strDeviceId){
    $device = new Device;
    $device->id =$strDeviceId;
    $response = $device->getAutomap($this->user['username'],$this->user['password']);
    return Response::json($response);
  }
  /**
   * Copy a device parent and locaton in other device
   * @param string $strDeviceId device id to copy
   * @param string $strCopyId device id from copy
   * @return json with error boolean and text message
   */
  public function copyLocation($strDeviceId,$strCopyId){
    //get data for from copy one
    $device = null;
    $copyDevice = null;
    try{
      $device = Device::getById($strDeviceId,$this->user['username'],$this->user['password']);
    }catch(Exception $e){
      return Response::json(array('error'=>true,'message'=>$e->getMessage()));
    }
    try{
      $copyDevice = Device::getById($strCopyId,$this->user['username'],$this->user['password']);
    }catch(Exception $e){
      return Response::json(array('error'=>true,'message'=>$e->getMessage()));
    }
     
    if(isset($copyDevice->location['lon']) && !is_null($copyDevice->location['lon'])){
      $device->lon = $copyDevice->location['lon'];
    }
    if(isset($copyDevice->location['lat']) && !is_null($copyDevice->location['lat'])){
      $device->lat = $copyDevice->location['lat'];
    }
    if(isset($copyDevice->location['street']) && !is_null($copyDevice->location['street'])){
      $device->street = (empty($copyDevice->location['street']))?'':$copyDevice->location['street'];
    }
    if(isset($copyDevice->location['building']) && !is_null($copyDevice->location['building'])){
      $device->building = (empty($copyDevice->location['building']))?'':$copyDevice->location['building'];
    }
    if(isset($copyDevice->location['floor']) && !is_null($copyDevice->location['floor'])){
      $device->floor = (empty($copyDevice->location['floor']))?'':$copyDevice->location['floor'];
    }
    if(isset($copyDevice->location['room']) && !is_null($copyDevice->location['room'])){
      $device->room = (empty($copyDevice->location['room']))?'':$copyDevice->location['room'];
    }
    if(isset($copyDevice->parent) && !is_null($copyDevice->parent)){
      $folder=new Folder;
      $folder->id=$device->id;
      $deviceOldParent = null;
      if(isset($device->parent) && !is_null($device->parent)){
        $deviceOldParent=$device->parent['id'];
      }
      $response = $folder->setParent($deviceOldParent,$copyDevice->parent['id'],$this->user['username'],$this->user['password']);
      if($response['error']){
        return Response::json(array('error'=>true,'message'=>'Could not set the parent'));
      }
    }
    //pass data and use the necesary tool to make it works

    $response = $device->edit($this->user['username'],$this->user['password']);

    return Response::json($response);
  }
  /**
   * Removes user from device
   * @param string $strUsername user to remove (unsubscribe)
   * @param string $strDeviceId device to remove user from
   * @return json containig a message of either success or error
   */
  public function removeDevicePermission($strDeviceId,$strUsername){
    $device = new Device;
    $device->id = $strDeviceId;
    $response = $device->removeDevicePermission($strUsername,$this->user['username'],$this->user['password']);
    return Response::json($response);
  }
  /**
   * Given a device, gets the storage urls
   * @param  string $strDeviceId id of the device
   * @return json with error key, and url array if successfull, message on error.
   * @author Ignacio Cano <i.cano@gbh.com.do>
   */
  public function getStorageUrls($strDeviceId){
    $device = new Device;
    $device->id = $strDeviceId;
    $response = $device->getStorageUrls($this->user['username'],$this->user['password']);
    return Response::json($response);
  }

  public function getTransducersLastValue($strDeviceId){
    $device = new Device;
    $device->id = $strDeviceId;
    $response = $device->getTransducersLastValue($this->user['username'],$this->user['password']);
    return Response::json($response);
  }
}