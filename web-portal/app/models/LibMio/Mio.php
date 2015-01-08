<?php namespace LibMio;
class Mio {
  private $host = XMPP_HOST;
  private $toolsPath = '';

  /**
   * Do some config on instance creation
   * set cli tools path.
   */
  public function __construct(){
    $this->toolsPath = dirname(__FILE__).'/Tools/';
  }
  /**
   * Tries to authenticate an user against the xmpp
   * @param  string $username     the username
   * @param  string $password the user's password
   * @return mixed  true on success, array with error and message
   */
  public function auth($strUsername,$strPassword){
    $strUsername .= '@'.$this->host;
    $strAuthTool = $this->toolsPath.'mio_authenticate'. sprintf(' -u %s -p %s',$strUsername,$strPassword);
    $strLastOutput = exec($strAuthTool,$arrOutput,$intReturnCode);
    if($intReturnCode == 1){
      return array('error'=>false,'message'=>'Logged in');
    }
    return array('error'=>true,'message'=>'Could not log you in');
  }
  /**
  * Subscribe a user to a device
  * @param $strDeviceId device id
  * @param $strUsername user username
  * @param $strPassword user password
  * @return error bool variable and text message
  */
  public function subscribe($strDeviceId,$strUsername,$strPassword){
    $strUsername .= '@'.$this->host;
    $strSubscribeTool = $this->toolsPath. 'mio_subscribe'. sprintf(' -event "%s" -u %s -p %s',$strDeviceId,$strUsername,$strPassword);
    $strLastOutput = exec($strSubscribeTool,$arrOutput,$intReturnCode);
    if($intReturnCode == 1){
      return array('error'=>false,'message'=>'You have succesfully subscribed to this device');
    }
    return array('error'=>true,'message'=>'There is a problem with the tool');
  }
  /**
  * Usubscribe a user to a device
  * @param $strDeviceId device id
  * @param $strUsername user username
  * @param $strPassword user password
  * @return error bool variable and text message
  */
  public function unSubscribe($strDeviceId,$strUsername,$strPassword){
    $strUsername .= '@'.$this->host;
    $strUnSubscribeTool = $this->toolsPath. 'mio_unsubscribe'. sprintf(' -event "%s" -u %s -p %s',$strDeviceId,$strUsername,$strPassword);
    $strLastOutput = exec($strUnSubscribeTool,$arrOutput,$intReturnCode);
    if($intReturnCode == 1){
      return array('error'=>false,'message'=>'You have been successfully unsubscribed from this device');
    }
    return array('error'=>true,'message'=>'There is a problem with the tool');
  }
  /**
   * Create a Node
   * @param  string $strNode  node identifier
   * @param  string $strNodeName node name
   * @param  string $strUsername   user username
   * @param  string $strPassword   user password
   * @return array with error and message  
   */
  public function createNode($strNode,$strNodeName,$strUsername,$strPassword){
    $strUsername .= '@'.$this->host;
    $strCreateNodeTool = $this->toolsPath.'mio_node_create'. sprintf(' -event "%s" -title "%s" -u %s -p %s',$strNode,$strNodeName,$strUsername,$strPassword);
    $strLastOutput = exec($strCreateNodeTool,$arrOutput,$intReturnCode);
    if($intReturnCode == 1){
      return array('error'=>false,'message'=>'Node created');
    }
    return array('error'=>true,'message'=>'Could not create node');
  }
  /**
   * Add node type
   * @param  string $strNode  node identifier
   * @param  string $strName node name
   * @param  bool $isLocationNode node name
   * @param  string $strUsername   user username
   * @param  string $strPassword   user password
   * @return array with error and message  
   */
  public function nodeType($strNode,$strName,$isLocationNode = true,$strUsername,$strPassword,$strInfo = null){
    $strUsername .= '@'.$this->host;
    $strNodeTypeTool = $this->toolsPath.'mio_meta_add'. sprintf(' -event "%s" -name "%s" -type %s -u %s -p %s',$strNode,$strName,$isLocationNode? 'location' : 'device',$strUsername,$strPassword);
    if(!is_null($strInfo)){
      $strNodeTypeTool .= sprintf(' -info "%s"',$strInfo);
    }
    $strLastOutput = exec($strNodeTypeTool,$arrOutput,$intReturnCode);
    if($intReturnCode == 1){
      return array('error'=>false,'message'=>'Node type assigned');
    }
    return array('error'=>true,'message'=>'Could not assign node type');
  }

  /**
   * Geo locate the Node
   * @param  string $strNode     Node identifier
   * @param  string $strLon      Longitude lon
   * @param  string $strLat      Latitude  lat
   * @param  string $strUsername User username
   * @param  string $strPassword User password
   * @param  string $strStreet   Street
   * @param  string $strBuilding Building
   * @param  string $strFloor    Floor
   * @param  string $strRoom     Room
   * @return array with boolean variable and text message
   *  @todo make all the fields lon and lat 
   */
  public function geoLocation($strNode,$strLon,$strLat,$strUsername,$strPassword,$strStreet = null,$strBuilding = null,$strFloor = null,$strRoom = null){
    $strUsername .= '@'.$this->host;
    $strGeoTool = $this->toolsPath.'mio_meta_geoloc_add'. sprintf(' -event "%s" -lon "%s" -lat "%s" -u %s -p %s',$strNode,$strLon,$strLat,$strUsername,$strPassword);
    if(!is_null($strStreet)){
      $strGeoTool .= sprintf(' -street "%s"',$strStreet);
    }
    if(!is_null($strBuilding)){
      $strGeoTool .= sprintf(' -building "%s"',$strBuilding);
    }
    if(!is_null($strFloor)){
      $strGeoTool .= sprintf(' -floor "%s"',$strFloor);
    }
    if(!is_null($strRoom)){
      $strGeoTool .= sprintf(' -room "%s"',$strRoom);
    }
    $strLastOutput = exec($strGeoTool,$arrOutput,$intReturnCode);
    if($intReturnCode == 1){
      return array('error'=>false,'message'=>'Success change');
    }
    return array('error'=>true,'message'=>'Could not assign new location');
  }
  /**
   * Update folder data
   * @param  string $strCollectionNode Collection node identifier
   * @param  string $strName node name
   * @param  string $strUsername   user username
   * @param  string $strPassword   user password 
   * @return array with a error bolean variable and text message
   */
  public function updateCollection($strCollectionNode,$strName,$strUsername,$strPassword){    
    return $this->nodeType($strCollectionNode,$strName,true,$strUsername,$strPassword);
  }
   /**
   * Update folder data
   * @param  string $strNode  node identifier
   * @param  string $strName  node name
   * @param  string $strUsername   user username
   * @param  string $strPassword   user password 
   * @return array with a error bolean variable and text message
   * @todo  edit info of device
   */
  public function editNode($strNode,$strName,$strUsername,$strPassword){
    return $this->nodeType($strNode,$strName,false,$strUsername,$strPassword);
  }
  /**
   * Delete a collection node
   * @param  string $strNode node identifier
   * @param  string $strUsername user username
   * @param  string $strPassword user password
   * @return array with a error bolean variable adn text message
   */
  public function deleteNode($strNode,$strUsername,$strPassword){
    $arrReferences = $this->getNodeReferences($strNode,$strUsername,$strPassword);
    if(!$arrReferences['error']){
      foreach($arrReferences['references'] as $node){
        if($node['relation'] == 'parent'){
          $this->removeNodeFromCollection($strNode,$node['id'],$strUsername,$strPassword); //don't care about output
        }
      }
    }
    $strUsername .= '@'.$this->host;
    $strDeleteNodeTool = $this->toolsPath.'mio_node_delete'. sprintf(' -event "%s" -u %s -p %s',$strNode,$strUsername,$strPassword);
    $strLastOutput = exec($strDeleteNodeTool,$arrOutput,$intReturnCode);
    if($intReturnCode == 1){
      return array('error'=>false,'message'=>'Node deleted');
    }
    return array('error'=>true,'message'=>'Could not delete node');
  }
  /**
   * Add a property to a specific node
   * @param string $strNodeId       Node identifier
   * @param string $strNameProperty Name of the property
   * @param string $strValue        Value of the property
   * @param string $strUsername     User username
   * @param string $strPassword     User password
   */
  public function addProperty($strNodeId,$strNameProperty,$strValue,$strUsername,$strPassword){
    $strUsername .= '@'.$this->host;
    $strAddPropertyTool = $this->toolsPath.'mio_meta_property_add'. sprintf(' -event "%s" -name %s -value %s -u %s -p %s',$strNodeId,$strNameProperty,$strValue,$strUsername,$strPassword);
    $strLastOutput = exec($strAddPropertyTool,$arrOutput,$intReturnCode);
    if($intReturnCode == 1){
      return array('error'=>false,'message'=>'The property '.$strNameProperty.' was added !');
    }
    return array('error'=>true,'message'=>'Could not set the ' .$strNameProperty. ' property');
  }
  /**
   * Get all meta data from a device
   * @param  string $strDeviceName device name
   * @param  string $strUsername   User username
   * @param  string $strPassword   User password
   * @return array error boolean variable and mixed text message or array favorites
   * @todo  implement mio tools and filter the data
   */
  public function getNodeMetaData($strDeviceName,$strUsername,$strPassword){
    // $strUsername .= '@'.$this->host;
    // $strAuthTool = $this->toolsPath.'mio_metadata'. sprintf('-event "%s" -u %s -p %s',$strDeviceName,$strUsername,$strPassword);
    // $strLastOutput = exec($strAuthTool,$arrOutput,$intReturnCode);

    return array(
        'error'=>false,
        'favorites'=>array(
          array(
            'name'=>'Lightbuls',
            'id'=>'2'
          ),
          array(
            'name'=>'AC',
            'id'=>'7'
          )
        )
      );
  }
  /**
   * Find sub nodes from a parent Node 
   * @param  string $strNode collection node identifier
   * @param  string $strUsername       user username
   * @param  string $strPassword       user password
   * @return array Folder data 
   * @todo  type of node, is it a device or a ref node
   */
  public function getNodeReferences($strNode,$strUsername,$strPassword){
    $strUsername .= '@'.$this->host;
    $strGetReferencesTool = $this->toolsPath.'mio_item_query_stanza'. sprintf(' -event "%s" -item references -u %s -p %s',$strNode,$strUsername,$strPassword);
    $strLastOutput = exec($strGetReferencesTool,$arrOutput,$intReturnCode);
    if($intReturnCode == 1){
      if(count ($arrOutput) < 1 ){
        return array('error'=>false,'references'=>array());
      }
      $arrRawReferences = json_decode(json_encode(simplexml_load_string($arrOutput[0])),true);
      if(empty($arrRawReferences['references'])){
        return array('error'=>false,'references'=>array());
      }
      if(isset($arrRawReferences['references']['reference']['@attributes'])){
        $arrRawReferences['references']['reference'] = array(0=>$arrRawReferences['references']['reference']);
      }
      $arrReferences = array();
      foreach($arrRawReferences['references']['reference'] as $arrReference){
        $arrReferences[] = array(
          'id'=>$arrReference['@attributes']['node'],
          'name'=>isset($arrReference['@attributes']['name']) ? $arrReference['@attributes']['name']: $arrReference['@attributes']['node'],
          'relation'=>$arrReference['@attributes']['type'],
          'type'=>$arrReference['@attributes']['metaType']
          );
      }
      return array('error'=>false,'references'=>$arrReferences);
    }
    return array('error'=>true,'message'=>'Could not get children');
  }
   /**
   * Find sub nodes from a parent Node 
   * @param  string $strEvent     node identifier
   * @param  string $strUsername  user username
   * @param  string $strPassword  user password
   * @return array list of device id or error message
   * @todo  type of node, is it a device or a ref node
   */
  public function getAutomap($strEvent,$strUsername,$strPassword){
    // execute the tool
    $strUsername .= '@'.$this->host;
    $strGetAutomap = $this->toolsPath.'mio_automap'. sprintf(' -event "%s" -u %s -p %s',$strEvent,$strUsername,$strPassword);
    $strLastOutput = exec($strGetAutomap,$arrOutput,$intReturnCode);
    //validate there's no error
    if($intReturnCode == 1){
      if(count ($arrOutput) < 1){
        return array ('error'=>false,'devices'=>array());
      }
      $arrOutput = implode(' ', $arrOutput);
      $arrRawDevices=json_decode(json_encode(simplexml_load_string($arrOutput)),true);
      $arrDevices = array();
      if(isset($arrRawDevices['@attributes'])){
        $arrRawDevices= array(0=>$arrRawDevices);
      }
      foreach ($arrRawDevices as $key=>$node) { 
        $arrDevices [] = $node['@attributes']['uuid'];
      }
      return array ('error'=>false,'devices'=>$arrDevices);
    }
    return array('error'=>true,'message'=>'Could not get similar devices');
  }
  /**
   * Add a node to a collection
   * @param string $strChildNode device name
   * @param string $strParentNode collection node identifier
   * @param string $strUsername user username
   * @param string $strPassword user password
   * @param bool $boolTwoWay add ref to child?
   * @return  array with a bolean var error and text message
   */
  public function addNodeToCollection($strChildNode,$strParentNode,$strUsername,$strPassword,$boolTwoWay=true){
    $strUsername .= '@'.$this->host;
    $strAddChildrenTool = $this->toolsPath.'mio_reference_child_add'. sprintf(' -child "%s" -parent "%s" -u %s -p %s',$strChildNode,$strParentNode,$strUsername,$strPassword);
    if($boolTwoWay){
      $strAddChildrenTool .=' -add_ref_child';
    }
    $strLastOutput = exec($strAddChildrenTool,$arrOutput,$intReturnCode);
    if($intReturnCode == 1){
      return array('error'=>false,'message'=>'Node added');
    }
    return array('error'=>true,'message'=>'Could not add node as a child');
  }
  /**
   * Remove a Node from a collection
   * @param  string $strChildNode  device name
   * @param  string $strParentNode  collection node identifier
   * @param  string $strUsername  user username
   * @param  string $strPassword  user password
   * @return array  array with a bolean var error and text message
   * @todo  implements the tools to remove a node form a collection            
   */
  public function removeNodeFromCollection($strChildNode,$strParentNode,$strUsername,$strPassword){
    $strUsername .= '@'.$this->host;
    $strRemoveChildTool = $this->toolsPath.'mio_reference_child_remove'. sprintf(' -child "%s" -parent "%s" -u %s -p %s',$strChildNode,$strParentNode,$strUsername,$strPassword);
    $strLastOutput = exec($strRemoveChildTool,$arrOutput,$intReturnCode);
    if($intReturnCode == 1){
      return array('error'=>false,'message'=>'Node removed');
    }
    return array('error'=>true,'message'=>'Could not remove node from parent');
  }
  /**
   * Return the nodes who's belong to a collection node
   * @param  string $strCollectionNode collection node identifier
   * @param  string $strUsername user username
   * @param  string $strPassword user password
   * @return array device data or array error
   * @todo  Remove, and check controller that uses this, it should use the getcollectionnodes
   */
  public function getNodesFromCollection($strCollectionNode,$strUsername,$strPassword){

      return array(
        'error'=>false,
        'objects'=>
          array(
            array(
              'id'=>'1',
              'name'=>'Firefly',
            ),
            array(
              'id'=>'2',
              'name'=>'BACNet',
            ),
            array(
              'id'=>'3',
              'name'=>'Climate Control',
            ),
            array(
              'id'=>'4',
              'name'=>'Wishtat',
            )
          )
        );
  }
  /**
   * @param string $strNode node to get affiliations off, if null, returns usernames affiliations
   * @param string $strUsername 
   * @param string strPassword
   * @return array with error message or list of affiliations on success
   * @todo stanza param, and parse xml
   */
  public function affiliationsQuery($strNode = null,$strUsername,$strPassword){
    $strUsername .= '@'.$this->host;
    $strAclAffiliationsTool = $this->toolsPath.'mio_acl_affiliations_query'. sprintf(' -u %s -p %s',$strUsername,$strPassword);
    if(!is_null($strNode)){
      $strAclAffiliationsTool .= sprintf(' -event "%s"',$strNode);
    }
    $strLastOutput = exec($strAclAffiliationsTool,$arrOutput,$intReturnCode);
    if($intReturnCode == 1){
      // start no stanza
      unset($arrOutput[0]);
      $arrAffiliations['affiliation'] = array();
      for($i = 1; $i<count($arrOutput);$i+=2){
        $arrPiecesJON = explode(':',$arrOutput[$i]);
        $arrPiecesAff = explode(':',$arrOutput[$i+1]);
        $strAffiliation = strtolower(trim($arrPiecesAff[1]));
        $arrAffiliation = array('@attributes'=>array(
            'affiliation'=>$strAffiliation,
          ));
        if(!is_null($strNode)){
          $arrAffiliation['@attributes']['jid']= trim($arrPiecesJON[1]);
        }else{
          $arrAffiliation['@attributes']['node']= trim($arrPiecesJON[1]);
          $arrAffiliation['@attributes']['nodeName']= trim($arrPiecesJON[1]);
        }
        $arrAffiliations['affiliation'][] = $arrAffiliation;
      }
      //end no stanza
      // With stanza
      // if($arrOutput[0] == '(null)'){
      //   return array('error'=>false,'affiliations'=>array());
      // }
      // $arrAffiliations = json_decode(json_encode(simplexml_load_string($arrOutput[0])),true);
      // if(count($arrAffiliations['affiliation']) == 1){
      //   $arrAffiliations['affiliation'] = array(0=>$arrAffiliations['affiliation']);
      // }
      return array('error'=>false,'affiliations'=>$arrAffiliations);
    }
    return array('error'=>true,'message'=>'Could not get permitted devices');
  }
  /**
   * @param string $strNode node to get affiliations off, if null, returns usernames subscription
   * @param string $strUsername 
   * @param string strPassword
   * @return array with error message or list of subscription on success
   * @todo stanza param, and parse xml
   */
  public function subscriptionsQuery($strNode = null,$strUsername,$strPassword){
    $strUsername .= '@'.$this->host;
    $strSubscriptionQueryTool = $this->toolsPath.'mio_subscriptions_query'. sprintf(' -u %s -p %s ',$strUsername,$strPassword);
    if(!is_null($strNode)){
      $strSubscriptionQueryTool .= sprintf(' -event "%s"',$strNode);
    }
    $strLastOutput = exec($strSubscriptionQueryTool,$arrOutput,$intReturnCode);
    if($intReturnCode == 1){
      $arrSubscriptions=array();
      for($i=2;$i<count($arrOutput);$i+=2){
        $arrSubscriptions[preg_replace('/\s+/','',str_replace('Subscription: ','',$arrOutput[$i-1]))] =array(
          'id'=>preg_replace('/\s+/','',str_replace('Subscription: ','',$arrOutput[$i-1])),
          'subid'=>preg_replace('/\s+/','',str_replace('Sub ID:','',$arrOutput[$i]))
        ); 
      }
      if(count($arrSubscriptions) <= 0){
        return array('error'=>false,'subscriptions'=>array());
      }
      return array('error'=>false,'subscriptions'=>$arrSubscriptions);
    }
    return array('error'=>true,'message'=>'Could not get subscriptions devices');
  }
  public function addDevicePermission($strPublisher,$strDeviceName,$strUsername,$strPassword){
    $strUsername .= '@'.$this->host;
    $strPublisher .='@'.$this->host;
    $strAclPublisherAddTool = $this->toolsPath.'mio_acl_publisher_add'. sprintf(' -publisher "%s" -event "%s" -u %s -p %s',$strPublisher,$strDeviceName,$strUsername,$strPassword);
    $strLastOutput = exec($strAclPublisherAddTool,$arrOutput,$intReturnCode);
    if($intReturnCode == 1){
      return array('error'=>false,'message'=>'Permission added');
    }
    return array('error'=>true,'message'=>'Could not add permission');
  }
  public function removeDevicePermission($strPublisher,$strDeviceName,$strUsername,$strPassword){
    $strUsername .= '@'.$this->host;
    $strPublisher .='@'.$this->host;
    $strAclPublisherRemoveTool = $this->toolsPath.'mio_acl_publisher_remove'. sprintf(' -publisher "%s" -event "%s" -u %s -p %s',$strPublisher,$strDeviceName,$strUsername,$strPassword);
    $strLastOutput = exec($strAclPublisherRemoveTool,$arrOutput,$intReturnCode);
    if($intReturnCode == 1){
      return array('error'=>false,'message'=>'Permission removed');
    }
    return array('error'=>true,'message'=>'Could not remove permission');
  }
  /**
   * Connects to the XMPP server to retrieve the list of registered users
   * @param string $strUsername user username
   * @param string $strPassword user password
   * @return array containing either the users data on success or error message on failure
   * @todo Create connection to xmpp in order to retrieve real user data
   */
  public function getUsers($strUsername,$strPassword){
    $strUsernamesTool = sprintf('sudo ejabberdctl registered_users %s',$this->host);
    $strLastOutput = exec($strUsernamesTool,$arrOutput,$intReturnCode);
    $arrUsers = array();
    if($intReturnCode==0) {
      foreach($arrOutput as $username) {
        $strUserDataTool = sprintf('sudo ejabberdctl private_get %s %s user http://mortar.io/user',$username,$this->host);
        $xmlOutput = '';
        $strLastOutput = exec($strUserDataTool,$xmlOutput,$intReturnCode);
        if($intReturnCode==0){
          $xml = simplexml_load_string($xmlOutput[0]);
          $returnUser = array();
          $returnUser['username'] = $username;
          if(isset($xml->name) && isset($xml->email) && isset($xml->group)){
            $returnUser['name'] = (string) $xml->name;
            $returnUser['email'] = (string) $xml->email;
            $returnUser['group'] = (string) $xml->group;
            $arrUsers[] = $returnUser;
          }
        }else{
          return array('error'=>true,'message'=>'Something went wrong with the tool.');
        }
      }
      return array('error'=>false, 'users'=>$arrUsers);
    }    
    return array('error'=>true,'message'=>'Something went wrong with the tool.');    
  }

  /**
   * Get the data of a user given its ID
   * @param int $intId ID of the user to retrieve 
   * @param string $strUsername current user's username 
   * @param string $strPassword current user's password 
   * @return JSON containing the user data
   */
  public function displayUser($strUsername){
    $strUserDataTool = sprintf('sudo ejabberdctl private_get %s %s user http://mortar.io/user',$strUsername,$this->host);
    $strLastOutput = exec($strUserDataTool,$xmlOutput,$intReturnCode);
    if($intReturnCode==0) {
      $xml = simplexml_load_string($xmlOutput[0]);
      $arrUser = array();
      if(isset($xml->name) && isset($xml->email) && isset($xml->group)){
        $arrUser['username'] = $strUsername;
        $arrUser['name'] = (string) $xml->name;
        $arrUser['email'] = (string) $xml->email;
        $arrUser['group'] = (string) $xml->group;
        return array('error'=>false,'user'=>$arrUser);
      }else{
        return array('error'=>true, 'message'=>'No user metadata associated.');
      }
    }
    return array('error'=>true,'message'=>'Something went wrong with the tool.');
  }

  /**
   * Creates a user from array of data
   * @param array @arrData data to create user
   * @return array containing message of either success or error
   */
  public function createUser($strUsername,$strPassword,$strName,$strEmail,$strGroup){
    // Register user on server using username, host and password
    $strCreateUserTool = sprintf('sudo ejabberdctl register %s %s %s',$strUsername,$this->host,$strPassword);
    exec($strCreateUserTool,$arrOutput,$intReturnCode);

    // If user registration succeeded then add user metadata
    if($intReturnCode==0){
      $strXmlData =  sprintf("<user xmlns='http://mortar.io/user'><name>%s</name><email>%s</email><group>%s</group></user>",$strName,$strEmail,$strGroup);
      $strAddMetaTool = sprintf('sudo ejabberdctl private_set %s %s ',$strUsername,$this->host);
      $strAddMetaTool .= MIO_ESCAPE.$strXmlData.MIO_ESCAPE;
      $lastLine = exec($strAddMetaTool,$arrOutput,$intReturnCode);
      if($intReturnCode==0){
        return array('error'=>false, 'message'=>'User successfully registered.');
      }else{
        return array('error'=>true, 'message'=>'Unable to save user metadata. '.$lastLine);        
      }
    }
    return array('error'=>true, 'message'=>'User could not be created. '.$arrOutput[0]);
  }

  /**
   * Updates an user
   * @param array @arrData data to update
   * @return array containing message of either success or error
   */
  public function editUser($strUsername,$strPassword=null,$strName,$strEmail,$strGroup){ 
    // Update user metadata   
    $strXmlData =  sprintf("<user xmlns='http://mortar.io/user'><name>%s</name><email>%s</email><group>%s</group></user>",$strName,$strEmail,$strGroup);
    $strEditMetaTool = sprintf('sudo ejabberdctl private_set %s %s ',$strUsername,$this->host);
    $strEditMetaTool .= MIO_ESCAPE.$strXmlData.MIO_ESCAPE;
    $lastLine = exec($strEditMetaTool,$arrOutputAddMeta,$intAddMetaReturnCode);

    // If user meta updated and password is passed, edit password
    if($intAddMetaReturnCode==0){
      if($strPassword!=null){
        $arrSetPassword = $this->setUserPassword($strUsername,$strPassword);
        if(!$arrSetPassword['error']){
          return array('error'=>false, 'message'=>'User successfully updated.');        
        }else{
          return array('error'=>true, 'message'=>'User password could not be updated.');
        }
      }else{
        return array('error'=>false, 'message'=>'User successfully updated.');
      }        
    }else{
      return array('error'=>true, 'message'=>'User could not be updated.');
    }
  }

  /**
   * Given a username and a password, sets the password for that username
   * @param string $strUsername username to change the password
   * @param string $strPassword new password for the given user
   * @return  array with error and message
   */
  public function setUserPassword($strUsername,$strPassword){
    $strChangePwdTool = sprintf('sudo ejabberdctl change_password %s %s %s',$strUsername,$this->host,$strPassword);
    $lastLineChangePwd = exec($strChangePwdTool,$arrOutputChangePwd,$intChangePwdReturnCode); 
    if($intChangePwdReturnCode==0){
      return array('error'=>false, 'message'=>'User password changed');        
    }
    return array('error'=>true, 'message'=>'User password could not be changed');
  }

  /**
   * Deletes an user from xmpp
   * @param $strUsername user to delete username
   * @return array containing message of either success or error
   */
  public function deleteUser($strUsername){
    $strDeleteTool = 'sudo ejabberdctl unregister '.$strUsername.' '.$this->host;
    $strDeleteLastOutput = exec($strDeleteTool,$arrDeleteOutput,$intDeleteReturnCode);

    // Check if user account still exists
    $strCheckAccountTool = 'sudo ejabberdctl check_account '.$strUsername.' '.$this->host;
    $strCheckAccountLastOutput = exec($strCheckAccountTool,$arrCheckOutput,$intCheckReturnCode);

    if($intCheckReturnCode==1){
      return array('error'=>false,'message'=>'User account successfully deleted.');
    }
    return array('error'=>true,'message'=>'User account could not be deleted.');
  }
  
  /**
   * Executes the cmdline tool to actuate, and passes the necesary params.
   * @param  string $strEvent    name of the node
   * @param  string $strValue    value code
   * @param  string $strUsername 
   * @param  string $strPassword 
   * @return mixed array with success or error and message.
   */
  public function actuate($strEvent,$strTransducer,$strValue,$strUsername,$strPassword){
    $strUsername .= '@'.$this->host;
    $strActuateTool = $this->toolsPath.'mio_actuate'. sprintf(' -event "%s" -id "%s" -value %s -u %s -p %s',$strEvent,$strTransducer,$strValue,$strUsername,$strPassword);
    $strLastOutput = exec($strActuateTool,$arrOutput,$intReturnCode);
    if($intReturnCode == 1 && isset($arrOutput[1]) && trim($arrOutput[1]) == "Request successful"){
      return array('error'=>false,'output'=>$arrOutput);
    }
    return array('error'=>true,'message'=>'could not actuate');
  }
  /**
   * Gets node meta data, splits output into diferent sections
   * @param  str $strEvent    Device to get meta data of
   * @param  str $strUsername
   * @param  str $strPassword
   * @return mixed array containing error
   * @todo  split the metadata into diferent arrays, for convenience
   */
  public function nodeMetaQuery($strEvent,$strUsername,$strPassword){
    $strUsername .= '@'.$this->host;
    $strMetaQTool = $this->toolsPath.'mio_item_query_stanza'. sprintf(' -event "%s" -item meta -u %s -p %s',$strEvent,$strUsername,$strPassword);
    $strLastOutput = exec($strMetaQTool,$arrOutput,$intReturnCode);
    if($intReturnCode == 1){
      if(empty($arrOutput)){
        return array('error'=>true,'message'=>'Has no metadata');
      }
      return array('error'=>false,'output'=>json_decode(json_encode(simplexml_load_string($arrOutput[0])),true));
    }
    return array('error'=>true,'message'=>'could not get meta data');
  }

  /**
   * Gets node data, splits output into diferent sections
   * @param  str $strEvent Device to get data of
   * @param  str $strUsername
   * @param  str $strPassword
   * @return mixed array containing error
   */
  public function getTransducerLastValue($strEvent,$strUsername,$strPassword){
    $strUsername .= '@'.$this->host;
    $strTransducerTool = $this->toolsPath. 'mio_item_query_stanza'. sprintf(' -event "%s" -item data -u %s -p %s',$strEvent,$strUsername,$strPassword);
    $strLastOutput = exec($strTransducerTool,$arrOutput,$intReturnCode);
    if($intReturnCode == 1 && !empty($arrOutput)){
      return array('error'=>false,'data'=>json_decode(json_encode(simplexml_load_string($arrOutput[0])),true));
    }
    return array('error'=>true,'message'=>'could not get meta data');
  }

  /**
   * Uses mio tools to get collection nodes of an user
   * @param string $strUsername user username whose collection nodes we want to retrieve
   * @return json containing the folders of the user or error
   * @todo Implement tool to get the actual folders of an user
   */
  public function getFolders($strUsername){    
    return array(
      array(
        'name'=>'Floor 1',
        'id'=>'floor_1'
        ),
      array(
        'name'=>'Floor 2',
        'id'=>'floor_2'
        ),
      array(
        'name'=>'Floor 3',
        'id'=>'floor_3'
        ),
      array(
        'name'=>'Floor 4',
        'id'=>'floor_4'
        )
    );          
  }
  /**
   * Gets historical data of a device
   * @param  string $strDeviceName device from which retrieve data
   * @param  string $strTransducer transducer identifier
   * @param  string $strStart      start date
   * @param  string $strEnd        end date
   * @return array containing device data
   * @todo Use mio to get devices' actual transducers
   */
  public function getTransducerTimeseries($strDeviceName,$strTransducer,$strStart,$strEnd,$strUsername,$strPassword){
    $device = array(
      'name'=>'XDHD',
      'type'=>'Thermostat',
      'serialNumber'=>'001.987',
      'transducers'=>array(
        array(
          'name'=>'temperature',          
          'data'=>array(
            array(
              'value'=>70,
              'timestamp'=>'2014-07-15'
              ),
              array(
                'value'=>76,
                'timestamp'=>'2014-07-16'
                ),
              array(
                'value'=>65,
                'timestamp'=>'2014-07-17'
                ),
              array(
                'value'=>70,
                'timestamp'=>'2014-07-20'
                ),
              array(
                'value'=>71,
                'timestamp'=>'2014-07-21'
                ),
              array(
                'value'=>70,
                'timestamp'=>'2014-07-22'
                )
              ),
          ),
        array(
          'name'=>'set_point',
          'data'=>array(
            array(
              'value'=>'10',
              'timestamp'=>'2014-07-15'
              ),
              array(
                'value'=>'15',
                'timestamp'=>'2014-07-16'
                ),
              array(
                'value'=>'20',
                'timestamp'=>'2014-07-17'
                ),
              array(
                'value'=>'10',
                'timestamp'=>'2014-07-20'
                ),
              array(
                'value'=>'25',
                'timestamp'=>'2014-07-21'
                ),
              array(
                'value'=>'19',
                'timestamp'=>'2014-07-22'
                ),
              )
          ),
        array(
          'name'=>'mode',
          'data'=>array(
            array(
              'value'=>'0',
              'timestamp'=>'2014-07-15'
              ),
              array(
                'value'=>'1',
                'timestamp'=>'2014-07-16'
                ),
              array(
                'value'=>'2',
                'timestamp'=>'2014-07-17'
                ),
              array(
                'value'=>'0',
                'timestamp'=>'2014-07-20'
                ),
              array(
                'value'=>'2',
                'timestamp'=>'2014-07-21'
                ),
              array(
                'value'=>'1',
                'timestamp'=>'2014-07-22'
                ),
              )
          )
        ));

    $arrTransducer = array();
    foreach ($device['transducers'] as $transducer) {
      if($transducer['name']==$strTransducer){
        $arrTransducer = $transducer;
        break;
      }
    }
    $arrValues = array();
    if(isset($arrTransducer['data'])){
      foreach ($arrTransducer['data'] as $data) {
        if($data['timestamp']>=$strStart && $data['timestamp']<=$strEnd){
          $arrValues[] = array('x'=>strtotime($data['timestamp'])*1000,'y'=>$data['value']);
        }
      }
      return array('error'=>false,'data'=>$arrValues);
    }    
    return array('error'=>true,'message'=>'Could not display device timeseries');
  }
  /**
   * Given an username and token, saves it to the xmpp user
   * @param string $strUsername Username which needs a recovery token created
   * @param string $strToken    [description]
   * @return array with error key and message, on success also returns the used token.
   */
  public function setRecoveryCode($strUsername,$strToken){
    $strRecoveryXml = "<recovery xmlns='http://mortar.io/user_recovery'><token>$strToken</token></recovery>";
    $strRecoveryCreateTool = sprintf("sudo ejabberdctl private_set %s %s ",$strUsername,$this->host);
    $strRecoveryCreateTool .= MIO_ESCAPE.$strRecoveryXml.MIO_ESCAPE;
    exec($strRecoveryCreateTool,$arrOutput,$intReturnCode);
    if($intReturnCode==0){
      return array('error'=>false,'message'=>'Created token','token'=>$strToken);
    }
    return array('error'=>true,'message'=>'Could not save created token');
  }
  /**
   * Gets the recovery token for a given username
   * @param  string $strUsername users whos token we need to retrieve
   * @return array with error key and message, on success has retrieved token
   */
  public function getRecoveryCode($strUsername){
    $strRecoveryGetTool = sprintf("sudo ejabberdctl private_get %s %s %s '%s'",$strUsername,$this->host,'recovery','http://mortar.io/user_recovery');
    exec($strRecoveryGetTool,$arrOutput,$intReturnCode);
    if($intReturnCode==0 && count($arrOutput) > 0){
      $objRecoveryXml = simplexml_load_string($arrOutput[0]);
      if(isset($objRecoveryXml->token) && (string)$objRecoveryXml->token != ''){
        return array('error'=>false,'token'=>(string)$objRecoveryXml->token);
      }
      return array('error'=>true,'message'=>'User has no token');
    }
    return array('error'=>true,'message'=>'Could not get token');
  }
  /**
   * Retrieves list of monitors the user has
   * @param  string $strUsername the user whos monitors to retrieve
   * @param  string $strPassword user password
   * @return array with error message or list of monitors
   */
  public function listMonitors($strUsername,$strPassword){
    $strListMonitorTool = sprintf("sudo ejabberdctl private_get %s %s %s %s",$strUsername,$this->host,'monitors','http://mortar.io/monitors');
    exec($strListMonitorTool,$arrOutput,$intReturnCode);
    if($intReturnCode == 0){
      $objMonitorsXml = simplexml_load_string($arrOutput[0]);
      $arrMonitors = json_decode(json_encode($objMonitorsXml),true);
      if(empty($arrMonitors)){
        return array('error'=>false,'monitors'=>array());
      }
      return array('error'=>false,'monitors'=>isset($arrMonitors['monitor'][0]) ? $arrMonitors['monitor'] : array($arrMonitors['monitor']));
    }
    return array('error'=>true,'message'=>'Could not get monitors');
  }
  /**
   * Saves the list of monitors the user has
   * @param  array $arrMonitors list of monitors
   * @param  string $strUsername the user whos monitors to save
   * @param  string $strPassword user password
   * @return array with error or success message
   */
  public function saveListMonitors($arrMonitors,$strUsername,$strPassword){
    $strXmlMonitors = "<monitors xmlns='http://mortar.io/monitors'>";
    foreach ($arrMonitors as $monitor) {
      $strXmlMonitors .=  "<monitor><id>{$monitor['id']}</id><name>{$monitor['name']}</name></monitor>";
    }
    $strXmlMonitors .='</monitors>';
    $strSLMonitorTool = sprintf("sudo ejabberdctl private_set %s %s ",$strUsername,$this->host);
    $strSLMonitorTool .= MIO_ESCAPE.$strXmlMonitors.MIO_ESCAPE;
    exec($strSLMonitorTool,$arrOutput,$intReturnCode);
    if($intReturnCode == 0){
      return array('error'=>false,'message'=>'List saved');
    }
    return array('error'=>true,'message'=>'Could not save monitors list');
  }
  /**
   * Creates a monitor for the user
   * @param  string $strMonitor  xml representation of the monitor
   * @param  string $strUsername username that is creating the monitor
   * @param  string $strPassword password of the user
   * @return array with error or success message
   */
  public function saveMonitor($strId,$strMonitor,$strUsername,$strPassword){
    $strSaveMonitorTool = sprintf("sudo ejabberdctl private_set %s %s ",$strUsername,$this->host);
    $strSaveMonitorTool .= MIO_ESCAPE.$strMonitor.MIO_ESCAPE;    
    exec($strSaveMonitorTool,$arrOutput,$intReturnCode);
    if($intReturnCode==0){
      return array('error'=>false,'monitor'=>$strId);
    }
    return array('error'=>true,'message'=>'Could not save monitor');
  }
  /**
   * Gets the transducers of a given monitor
   * @param  string $strNS       namespace of the monitor
   * @param  string $strUsername owner of the monitor
   * @param  string $strPassword user password
   * @return array with error message or monitor xml string
   */
  public function getMonitor($strNS,$strUsername,$strPassword){
    $strGetMonitorTool = sprintf("sudo ejabberdctl private_get %s %s %s '%s'",$strUsername,$this->host,'monitor',$strNS);
    exec($strGetMonitorTool,$arrOutput,$intReturnCode);
    if($intReturnCode == 0){
      return array('error'=>false,'monitor'=>$arrOutput[0]);
    }
    return array('error'=>true,'message'=>'Could not retrieve monitor');
  }
  /**
   * Gets a device schedules
   * @param  string $strNode     device id
   * @param  string $strUsername username of the current user
   * @param  string $strPassword password of the current user
   * @return array with error message or array of schedules
   */
  public function scheduleQuery($strNode,$strUsername,$strPassword){
    $strUsername .= '@'.$this->host;
    $strSchedQueryTool = $this->toolsPath.'mio_item_query_stanza'. sprintf(' -event "%s" -item schedule -u %s -p %s',$strNode,$strUsername,$strPassword);
    $strLastOutput = exec($strSchedQueryTool,$arrOutput,$intReturnCode);
    if($intReturnCode == 1){
      if(count($arrOutput)< 1){
        return array('error'=>false,'schedules'=>array());
      }
      $arrRawSchedules = json_decode(json_encode(simplexml_load_string($arrOutput[0])),true);
      $arrSchedules = array();
      if(!isset($arrRawSchedules['schedule']['event'])){
        return array('error'=>false,'schedules'=>array());
      }
      if(isset($arrRawSchedules['schedule']['event']['@attributes'])){
        $arrRawSchedules['schedule']['event'] = array(0=>$arrRawSchedules['schedule']['event']);
      }
      foreach ($arrRawSchedules['schedule']['event'] as $arrSchedule) {
        if(!isset($arrSchedule['@attributes']['time'])){
          continue;
        }
            $arrSchedules[]=array(
            'id'=>$arrSchedule['@attributes']['id'],
            'time'=>$arrSchedule['@attributes']['time'],
            'info'=>isset($arrSchedule['@attributes']['info']) ? $arrSchedule['@attributes']['info'] : null,
            't_name'=>isset($arrSchedule['@attributes']['transducerName']) ? $arrSchedule['@attributes']['transducerName'] : null,
            't_value'=>isset($arrSchedule['@attributes']['transducerValue']) ? $arrSchedule['@attributes']['transducerValue'] : null,
            'freq'=>isset($arrSchedule['recurrence']['freq']) ? $arrSchedule['recurrence']['freq'] : null,
            'interval'=>isset($arrSchedule['recurrence']['interval']) ? $arrSchedule['recurrence']['interval'] : null,
            'count'=>isset($arrSchedule['recurrence']['count']) ? $arrSchedule['recurrence']['count'] : null,
            'until'=>isset($arrSchedule['recurrence']['until']) ? $arrSchedule['recurrence']['until'] : null,
            'bymonth'=>isset($arrSchedule['recurrence']['bymonth']) ? $arrSchedule['recurrence']['bymonth'] : null,
            'byday'=>isset($arrSchedule['recurrence']['byday']) ? $arrSchedule['recurrence']['byday'] : null,
            'exdate'=>isset($arrSchedule['recurrence']['exdate']) ? $arrSchedule['recurrence']['exdate'] : null
          );
        
      }
      //pr($arrSchedules);
      return array('error'=>false,'schedules'=>$arrSchedules);
    }
    return array('error'=>true,'message'=>'Could not get schedules');
  }
  /**
   * Saves a schedule in the xmpp
   * @param  string $strNode     node id
   * @param  array $arrSchedule [description]
   * @param  string $strUsername username of current user
   * @param  string $strPassword pass of current user
   * @return bool true on success, false otherwise
   */
  public function saveSchedule($strNode,$arrSchedule,$strUsername,$strPassword){
    $strUsername .= '@'.$this->host;
    $strSchedSaveTool = $this->toolsPath.'mio_schedule_event_add'. sprintf(' -event "%s" -u %s -p %s',$strNode,$strUsername,$strPassword);
    if(isset($arrSchedule['id'])){
      $strSchedSaveTool .= sprintf(' -id "%s"',$arrSchedule['id']);
    }
    if(isset($arrSchedule['time']) && !empty($arrSchedule['time'])){
      $strSchedSaveTool .= sprintf(' -time "%s"',$arrSchedule['time']);
    }
    if(isset($arrSchedule['info']) && !empty($arrSchedule['info'])){
      $strSchedSaveTool .= sprintf(' -info "%s"',$arrSchedule['info']);
    }
    if(isset($arrSchedule['t_name']) && !empty($arrSchedule['t_name'])){
      $strSchedSaveTool .= sprintf(' -t_name "%s"',$arrSchedule['t_name']);
    }
    if(isset($arrSchedule['t_value'])){
      $strSchedSaveTool .= sprintf(' -t_value "%s"',$arrSchedule['t_value']);
    }
    if(isset($arrSchedule['r_freq']) && !empty($arrSchedule['r_freq'])){
      $strSchedSaveTool .= sprintf(' -r_freq "%s"',$arrSchedule['r_freq']);
    }
    if(isset($arrSchedule['r_int']) && !empty($arrSchedule['r_int'])){
      $strSchedSaveTool .= sprintf(' -r_int "%s"',$arrSchedule['r_int']);
    }
    if(isset($arrSchedule['r_count']) && !empty($arrSchedule['r_count'])){
      $strSchedSaveTool .= sprintf(' -r_count "%s"',$arrSchedule['r_count']);
    }
    if(isset($arrSchedule['r_until']) && !empty($arrSchedule['r_until'])){
      $strSchedSaveTool .= sprintf(' -r_until "%s"',$arrSchedule['r_until']);
    }
    if(isset($arrSchedule['r_bymonth']) && !empty($arrSchedule['r_bymonth'])){
      $strSchedSaveTool .= sprintf(' -r_bymonth "%s"',implode(',',$arrSchedule['r_bymonth']));
    }else{
      $strSchedSaveTool .= ' -r_bymonth "0,1,2,3,4,5,6,7,8,9,10,11"';
    }
    if(isset($arrSchedule['r_byday']) && !empty($arrSchedule['r_byday'])){
      $strSchedSaveTool .= sprintf(' -r_byday "%s"',implode(',',$arrSchedule['r_byday']));
    }
    if(isset($arrSchedule['r_exdate']) && !empty($arrSchedule['r_exdate'])){
      $strSchedSaveTool .= sprintf(' -r_exdate "%s"',implode(',',$arrSchedule['r_exdate']));
    }
    $strLastOutput = exec($strSchedSaveTool,$arrOutput,$intReturnCode);
    if($intReturnCode == 1){
      return false;
    }
    return true;
  }
  /**
   * Deletes a schedule from a device
   * @param  string $strNode     node id
   * @param  string $strId       id of the schedule
   * @param  string $strUsername username of current user
   * @param  string $strPassword password of current user
   * @return array with error key, and message
   */
  public function removeSchedule($strNode,$strId,$strUsername,$strPassword){
    $strUsername .= '@'.$this->host;
    $strSchedRemoveTool = $this->toolsPath.'mio_schedule_event_remove'. sprintf(' -event "%s" -id %s -u %s -p %s',$strNode,$strId,$strUsername,$strPassword);
    $strLastOutput = exec($strSchedRemoveTool,$arrOutput,$intReturnCode);
    if($intReturnCode == 1){
      return array('error'=>false,'message'=>'Schedule removed');
    }
    return array('error'=>true,'message'=>'Could not remove schedule');
  }
  
  /**
   * Returns the storage item of a node in the xmpp
   * @param  string $strNode     node id
   * @param  string $strUsername username of current user
   * @param  string $strPassword password of current user
   * @return [type]              [description]
   * @author Ignacio Cano <i.cano@gbh.com.do>
   */
  public function getStorageUrls($strNode,$strUsername,$strPassword){
    $strUsername .= '@'.$this->host;
    $strStorageItemTool = $this->toolsPath.'mio_item_query_stanza'.sprintf(' -event "%s" -item storage -u %s -p %s',$strNode,$strUsername,$strPassword);
    $strLastOutput = exec($strStorageItemTool,$arrOutput,$intReturnCode);
    if($intReturnCode == 1){
      $arrUrls = array();
      if(!isset($arrOutput[0])){
        return array('error'=>false,'urls'=>$arrUrls);
      }
      // $arrOutput[0] = '<item id="storage"><addresses><address link="http://sensor.andrew.cmu.edu:8080"/></addresses></item>';
      $arrStorage = json_decode(json_encode(simplexml_load_string($arrOutput[0])),true);
      if(isset($arrStorage['addresses']['address'])){
        $arrStorage['addresses']['address'] = array(0=>$arrStorage['addresses']['address']);
      }
      foreach ($arrStorage['addresses']['address'] as $arrAddress) {
        $arrUrls[] = $arrAddress['@attributes']['link'];
      }
      return array('error'=>false,'urls'=>$arrUrls);
    }
    return array('error'=>true,'message'=>'could not get storage urls');
  }
}