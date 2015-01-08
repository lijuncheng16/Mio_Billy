<?php

class FolderController extends BaseController {
  /**
   * Add device to the current folder
   * @param string $strFolderId 
   * @return json with bolean error and a text message
   */
  public function addDevice($strFolderId){
    $strDeviceId = Input::get('device_id', null);
    $boolTwoWay = Input::get('two_way', true);
    if(is_null($strDeviceId)){
      return Response::json(array('error'=>true,'message'=>'Device id is needed'));
    }
    $folder = new Folder;
    $folder->id = $strFolderId;
    $response = $folder->addDevice($strDeviceId,$this->user['username'],$this->user['password'],$boolTwoWay);
    return Response::json($response);
  }
  /**
   * Remove a device from a folder
   * @param string $strFolderId  
   * @param string $strDeviceId
   * @return json with bolean error and text message
   */
  public function removeDevice($strFolderId,$strDeviceId){
    $folder = new Folder;
    $folder->id=$strFolderId;
    $response = $folder->removeDevice($strDeviceId,$this->user['username'],$this->user['password']);
    return Response::json($response);
  }
  /**
   * Return folders 
   * @param string $strFolderId 
   * @return mixed json folder data or json error and message
   */
  public function getChildren($strFolderId) {   
    $folder = new Folder;
    $folder->id = $strFolderId;
    $response = $folder->getChildren($this->user['username'],$this->user['password']);
    return Response::json($response);
  }
  /**
   * Set a parent to node, remove previous parent.
   * @param string $strFolderId 
   * @return  json with error variable and a text message
   * @todo  implement the tool and validation needed
   */
  public function setParent($strFolderId){
    $strOldParent = Input::get('old_parent', null);
    $strNewParent = Input::get('new_parent', null);
    if(is_null($strNewParent)){
      return Response::json(array('error'=>true,'message'=>'Parent folder is needed'));
    }
    $folder=new Folder;
    $folder->id=$strFolderId;
    $response = $folder->setParent($strOldParent,$strNewParent,$this->user['username'],$this->user['password']);
    return Response::json($response);
  } 
  /**
   * Create a Folder
   * @return mixed return true or array with error adn message
   */
  public function createFolder(){
    $strFolderName = Input::get('folder_name',null);
    $strFolderParent = Input::get('folder_parent',null);
    $strMapUriUrl = Input::get('mapUri_url');
    if(is_null($strFolderName) || is_null($strFolderParent)){
      return Response::json(array('error'=>true,'message'=>'Missing arguments'));
    }

    $folder = new Folder;
    $folder->id = UUID::generate(4)->string;
    $folder->name = $strFolderName;
    $folder->parent = $strFolderParent;
    if(isset($strMapUriUrl) && !empty($strMapUriUrl)){
      $folder->mapUri = $strMapUriUrl;
    }else if(Input::hasFile('folder_mapUri')){
      $folder->mapUri= $this->uploadFile('folder_mapUri');
    }
    $response = $folder->createFolder($this->user['username'],$this->user['password']);
    if(!$response['error']){
      $response['folder_id'] =(string) $folder->id;
    }
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
    if($file->move(public_path().'/img/folders/',$fileName) ){
      return '/img/folders/'.$fileName;
    }
    return null;
  }
  /**
   * Subscribe a user
   * @param string $strFolderId 
   * @return json with error variable and a text message
   * @todo  ask if we going to delete
   */
  public function subscribesUser($strFolderId){
    $arrData = Input::all();
    $folder= new Folder;
    if(!isset($arrData['username']) || empty($arrData['username'])){
      return Response::json(array('error'=>true,'message'=>'Missing arguments'));
    }
    $user= new User;
    $user->username=$arrData['username'];
    $user->password=$arrData['password'];
    $folder->id = $strFolderId;
    $response =$folder->subscribeUser($user->username,$user->password);
    return Response::json($response);
  }
  /**
   * Unsubscribe a user
   * @return json with error variable and a text message
   * @todo  ask if we going to delete
   */
  public function unsubscribeUser($strFolderId){
    $arrData = Input::all(); /** @todo use data of the select user not current user */
    if(!isset($arrData['username']) || empty($arrData['username'])){
      return array('error'=>true,'message'=>'Missing arguments');
    }
    $folder = new Folder;
    $user = new User;
    $user->username=$arrData['username'];
    $user->password=$arrData['password'];
    $folder->id= $strFolderId;
    $response = $folder->unsubscribeUser($user->username,$user->password);
    return Response::json($response);
  }
  /**
   * Delete a folder
   * @param  string $strFolderId 
   * @return json with bolean error variable and text message             
   */
  public function deleteFolder($strFolderId){
    $folder=new Folder;
    if($strFolderId == 'root' || strpos($strFolderId,'_favorites') !== false){
      return Response::json(array('error'=>true,'message'=>'This folder cannot be deleted, they are part of the system'));
    }
    $folder->id = $strFolderId;
    $response = $folder->deleteFolder($this->user['username'],$this->user['password']);
    return Response::json($response); 
  }
  /**
   * Update folder name and the map uri
   * @param string $strFolderId 
   * @return json with bolean error variable and text message
   */
  public function editFolder($strFolderId){
    $strFolderName = Input::get('folder_name',null);
    $strOldParent = Input::get('old_parent',null);
    $strFolderParent = Input::get('folder_parent',null);
    $strMapUriUrl = Input::get('mapUri_url');
    if(is_null($strFolderName) || is_null($strFolderParent)){
      return Response::json(array('error'=>true,'message'=>'Missing arguments'));
    }
    $folder = new Folder;
    $folder->id=$strFolderId;
    $folder->name = $strFolderName;
    $folder->oldParent = $strOldParent;
    $folder->parent = $strFolderParent;
    if(isset($strMapUriUrl) && !empty($strMapUriUrl)){
      $folder->mapUri = $strMapUriUrl;
    }else if(Input::hasFile('folder_mapUri')){
      $folder->mapUri= $this->uploadFile('folder_mapUri');
    }
    $response = $folder->editFolder($this->user['username'],$this->user['password']);
    return Response::json($response);
  }
  /**
   * return a list of device 
   * @param string $strFolderId 
   * @return mixed json folder data or json error and message
   * @todo  Recibir la data real
   */
  public function getDevices($strFolderId){
    $folder=new Folder;
    $folder->id=$strFolderId;
    $response=$folder->getDevices($this->user['username'],$this->user['password']);
    return Response::json($response);
  }
}