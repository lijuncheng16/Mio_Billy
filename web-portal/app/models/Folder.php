<?php

class Folder {
  /**
   * Return childs folders with his devices
   * @param  string $strUsername
   * @param  string $strPassword
   * @return array with error bolean and objects with folders data
   */
  public function getChildren($strUsername,$strPassword){
    if(empty($this->id)){
      return array('error'=>true,'message'=>'Missing argument');
    }
    $return = Mio::getNodeReferences($this->id,$strUsername,$strPassword);
    if($return['error']){
      return $return;
    }
    $arrReferences = $return['references'];
    foreach($arrReferences as $index => $node){
      if($node['relation'] == 'parent'){
        unset($arrReferences[$index]);
      }
      /**
       * Treat unknown as a device.. 
       * @todo they cant replicate this behaviour... so we can't get it fixed.
       */
      if($node['type'] == 'unknown'){
       $arrReferences[$index]['type'] = 'device';
      }
    }
    return array('error'=>false,'children'=>$arrReferences);
  }
  /**
   * add device in to the folder
   * @param string $strDeviceId id of the device to add
   * @param string $strUsername 
   * @param string $strPassword 
   * @param bool $boolTwoWay should it add the reference to the child aswell
   * @return array with bolean var error and text message
   */
  public function addDevice($strDeviceId,$strUsername,$strPassword,$boolTwoWay = true){
    return Mio::addNodeToCollection($strDeviceId,$this->id,$strUsername,$strPassword,$boolTwoWay);
  }
  /**
   * remove a device from a collection
   * @param  Object $strDeviceId
   * @param  string $strUsername 
   * @param  string $strPassword 
   * @return array with bolean var error and text message
   * @todo  Validar si el device enviando esta dentro del listado de device del folder
   */
  public function removeDevice($strDeviceId,$strUsername,$strPassword){
    return Mio::removeNodeFromCollection($strDeviceId,$this->id,$strUsername,$strPassword);
  }
  /**  
   * Set parent node
   * @param string $strUsername 
   * @param string $strPassword 
   * @return array with bolean var error and text message
   */
  public function setParent($strOldParent=null,$strNewParent,$strUsername,$strPassword){
    $arrAddResponse = Mio::addNodeToCollection($this->id,$strNewParent,$strUsername,$strPassword);
    if($arrAddResponse['error']){
      return $arrAddResponse;
    }
    if(!is_null($strOldParent)){
      $arrRemoveResponse = Mio::removeNodeFromCollection($this->id,$strOldParent,$strUsername,$strPassword);
      if($arrRemoveResponse['error']){
        $arrRemoveNewResponse = Mio::removeNodeFromCollection($this->id,$strNewParent,$strUsername,$strPassword);
        if($arrRemoveNewResponse['error']){
          return array('error'=>true,'message'=>'Something is going wrong. New parent was set but could not remove old one');          
        }
        return $arrRemoveResponse;
      }
    }      
    return $arrAddResponse;
  }
  /**
   * create a Folder
   * @param  string $strUsername 
   * @param  string $strPassword 
   * @return array with a bolean error and a text message
   */
  public function createFolder($strUsername,$strPassword){
    $return = Mio::createNode($this->id,$this->name,$strUsername,$strPassword);
    if($return['error']){
      return $return;
    }
    $return = Mio::nodeType($this->id,$this->name,true,$strUsername,$strPassword);
    if($return['error']){
      Mio::deleteNode($this->id,$strUsername,$strPassword);
      return array('error'=>true,'message'=>'Could not assign node type');
    }
    $return = $this->setParent(null,$this->parent,$strUsername,$strPassword);
    if($return['error']){
      Mio::deleteNode($this->id,$strUsername,$strPassword);
      return array('error'=>true,'message'=>'Could not save the parent of the node');
    }
    if(isset($this->mapUri) && !empty($this->mapUri) && !is_null($this->mapUri)){
      $return=$this->addProperty('mapUri',$this->mapUri,$strUsername,$strPassword);
      if($return['error']){
        return array('error'=>true,'message'=>'Could not add the map uri property');
      }
    }
    return $return;
  }
  /**
   * Add a property to the folder
   * @param string $strNameProperty Property name
   * @param string $strValue        Property value
   * @param string $strUsername     User username
   * @param string $strPassword     User password
   * @return array with boolean variable and text message
   */
  private function addProperty($strNameProperty,$strValue,$strUsername,$strPassword){
    $return = Mio::addProperty($this->id,$strNameProperty,$strValue,$strUsername,$strPassword);
    if($return['error']){
      return array('error'=>false,'message'=>'Could not save the'. $strNameProperty .' edit the folder to add');
    }
    return $return;
  }
  /**
   * Update folder data
   * @param  string $strUsername 
   * @param  string $strPassword 
   * @return array with error bolean variable and text message              
   */
  public function editFolder($strUsername,$strPassword){
    $return = array('error'=>false,'message'=>'There is nothing to change');
    if(isset($this->name) && !is_null($this->name) && !empty($this->name)){
      $return = Mio::updateCollection($this->id,$this->name,$strUsername,$strPassword);
      if($return['error']){
        return array('error'=>true,'message'=>'Could not change the name of the node');
      }
    }
    if(isset($this->oldParent) && !is_null($this->oldParent) && isset($this->parent) && !is_null($this->parent)){
      if($this->parent != $this->oldParent){
        $return = $this->setParent($this->oldParent,$this->parent,$strUsername,$strPassword);
        if($return['error']){
          return array('error'=>true,'message'=>'Could not change the parent of the node');
        }
      }
    }
    if(isset($this->mapUri) && !empty($this->mapUri) && !is_null($this->mapUri)){
      $return=$this->addProperty('mapUri',$this->mapUri,$strUsername,$strPassword);
      if($return['error']){
        return array('error'=>true,'message'=>'Could not edit the map uri property');
      }
    }
    return $return;
  }
  /**
   * Delete a folder
   * @param  string $strUsername  
   * @param  string $strPassword  
   * @return array with a bolean variable and text message
   */
  public function deleteFolder($strUsername,$strPassword){
    $return = Mio::deleteNode($this->id,$strUsername,$strPassword);
    return $return;
  }
  /**
   * Subscreibe a user to a folder
   * @param  Object $objUser
   * @param  string $strUsername 
   * @param  string $strPassword    
   * @return array with a bolean error and a text message
   */
  public function subscribesUser($strUsername,$strPassword){
    $return =Mio::subscribeUser($this->id,$strUsername,$strPassword);
    return $return;
  }
  /**
   * Unsubscreibe a user from a folder
   * @param  string $strUsername 
   * @param  string $strPassword    
   * @return array with a bolean error and a text message
   */
  public function unsubscribeUser($strUsername,$strPassword){
    $return =Mio::unsubscribeUser($this->id,$strUsername,$strPassword);
    return $return;
  }
  /**
   * Return a array of device
   * @param  string $strUsername 
   * @param  string $strPassword 
   * @return mixed array of device data or array with error and message
   */
  public function getDevices($strUsername,$strPassword){
    $return = Mio::getNodeReferences($this->id,$strUsername,$strPassword);
    if($return['error']){
      return $return;
    }
    $arrReferences = $return['references'];
    foreach($arrReferences as $index => $node){
      /**
       * Treat unknown as a device.. 
       * @todo they cant replicate this behaviour... so we can't get it fixed.
       */
      if($node['type'] == 'unknown'){
       $arrReferences[$index]['type'] = 'device';
       $node['type'] = 'device';
      }
      if($node['relation']== 'parent' || $node['type'] != 'device'){
        unset($arrReferences[$index]);
      }
      if($node['type']=='device'){
        try{
          $arrReferences[$index]['meta'] = Device::getById($node['id'],$strUsername,$strPassword);
        }catch(Exception $e){

        }
      }
    }
    return array('error'=>false,'children'=>$arrReferences);
  }
}