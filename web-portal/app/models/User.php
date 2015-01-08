<?php

class User {
  public $username;
  public $password;
  protected $name;
  protected $email;
  protected $group;

  /**
   * Creates an user object using data from array (must contain all of user attributes)
   * @param array $arrData array to build object from
   * @return mixed User object on success and exception on error
   */
  public static function fromArrayAll($arrData){
    $user = new self();
    if(!isset($arrData['username']) || empty($arrData['username'])){
      throw new Exception("No username provided");      
    }else{
      $user->username = $arrData['username'];
    }
    if(!isset($arrData['password']) || empty($arrData['password'])){
      throw new Exception("No password provided");      
    }else{
      $user->password = $arrData['password'];
    }
    if(!isset($arrData['password2']) || empty($arrData['password2'])){
      throw new Exception("No password confirmation provided");      
    }else{
      if($arrData['password2'] !== $arrData['password']){
        throw new Exception("Passwords do not match");
      }
    }
    if(!isset($arrData['name']) || empty($arrData['name'])){
      throw new Exception("No name provided");      
    }else{
      $user->name = $arrData['name'];
    }
    if(!isset($arrData['email']) || empty($arrData['email'])){
      throw new Exception("No email provided");      
    }else{
      $user->email = $arrData['email'];
    }
    if(!isset($arrData['group']) || empty($arrData['group'])){
      throw new Exception("No user type provided");      
    }else{
      $user->group = $arrData['group'];
    }
    return $user;
  }

  /**
   * Creates an user object using array data (it may contain either only username, name, email and group or all attributes)
   * @param array $arrData array to build object from 
   * @return mixed User object on success and exception on error
   */
  public static function fromArray($arrData){
    $user = new self();
    if(!isset($arrData['username']) || empty($arrData['username'])){
      throw new Exception("No username provided");      
    }else{
      $user->username = $arrData['username'];
    }
    if(!isset($arrData['name']) || empty($arrData['name'])){
      throw new Exception("No name provided");      
    }else{
      $user->name = $arrData['name'];
    }
    if(!isset($arrData['email']) || empty($arrData['email'])){
      throw new Exception("No email provided");      
    }else{
      $user->email = $arrData['email'];
    }
    if(!isset($arrData['group']) || empty($arrData['group'])){
      throw new Exception("No user type provided");      
    }else{
      $user->group = $arrData['group'];
    }

    if(isset($arrData['password'])) {
      if(empty($arrData['password'])){
        throw new Exception("No password provided");        
      }
      if(!isset($arrData['password2']) || empty($arrData['password2'])){
        throw new Exception("No password confirmation provided");      
      }else{
        if($arrData['password2'] !== $arrData['password']){
          throw new Exception("Passwords do not match");
        }else{
          $user->password = $arrData['password'];
        }
      }
    }
    return $user;
  }

	/**
	 * Logins the user, and gets extra info (group, nickname)
	 * @return array given error, or auth data
	 * @todo  get user extra info, return on errors (i can't determine this yet!)
	 */
	public function login(){

		$return = Mio::auth($this->username,$this->password);
    if($return['error']){
      return $return;
    }
    // if(XMPP_HOST == 'localhost'){
      $return = $this->displayUser($this->username);
    // }else{
    //   $arrUser = array();
    //   $arrUser['username'] = 'Luis';
    //   $arrUser['name'] = 'Luis';
    //   $arrUser['email'] = 'Luis.com';
    //   $arrUser['group'] = 'admin';
    //   $return =array('error'=>false,'user'=>$arrUser);
    // }
    
		return $return;
	}

	/**
	 * Uses mio to get a list of registered users
	 * @return array of registered users
	 */
	public function index(){
		$return = Mio::getUsers($this->username,$this->password);
		return $return;
	}
  /**
   * Uses mio lib to display user data
   * @param string $strUsername user username to display
   * @return array with either user data or error message
   */
	public function displayUser($strUsername){
		$return = Mio::displayUser($strUsername);
		return $return;
	}

  /**
   * Uses mio to create a new user in the xmpp server
   * @return array $return array with either success or error message
   */
  public function createUser(){
    $return = Mio::createUser($this->username,$this->password,$this->name,$this->email,$this->group);
    if($return['error']){
      return $return;
    }
    $strNode = $this->username.'_favorites';
    $strNodeName = 'Favorites';
    $arrNodeReturn = Mio::createNode($strNode,$strNodeName,$this->username,$this->password);
    if(!$arrNodeReturn['error']){
      Mio::nodeType($strNode,$strNodeName,true,$this->username,$this->password);
    }
    return $return;
  }

  /**
   * Uses mio to edit an user in the xmpp
   * @param array $arrData data to edit user
   * @return array $return array with either success or error message
   */
  public function edit(){
    $return = Mio::editUser($this->username,$this->password,$this->name,$this->email,$this->group,$this->password);
    return $return;
  }

  /**
   * Uses mio to delete an user from xmpp
   * @param strUsername  user username
   * @return json containig a message of either success or error 
   */
  public function deleteUser($strUsername){
    $return = Mio::deleteUser($strUsername);
    return $return;
  }

  /**
   * Gets all nodes to which user have permissions
   * @param string $strUsername current user
   * @param string $strPassword current user password
   */
	public function getPermittedDevices($strUsername,$strPassword){
    $return = Mio::affiliationsQuery(null,$strUsername,$strPassword);
    if($return['error']){
      return $return;
    }
    
    $arrDevices = array('publisher'=>array(),'owner'=>array(),'subscribe'=>array());
    
    if(isset($return['affiliations']['affiliation'])){
      foreach($return['affiliations']['affiliation'] as $arrAffiliation){
        if($arrAffiliation['@attributes']['affiliation'] == 'publisher'){
          $arrDevices['publisher'][$arrAffiliation['@attributes']['node']] = array(
          'id'=>$arrAffiliation['@attributes']['node'],
          // 'type'=>$nodeType,
          // 'name'=>$nodeTypeResponse['output']['meta']['@attributes']['name']
          );
        }
        if($arrAffiliation['@attributes']['affiliation'] == 'owner'){
          $arrDevices['owner'][$arrAffiliation['@attributes']['node']] = array(
          'id'=>$arrAffiliation['@attributes']['node'],
          // 'type'=>$nodeType,
          // 'name'=>$nodeTypeResponse['output']['meta']['@attributes']['name']
          );
        }
        // $nodeTypeResponse = Mio::nodeMetaQuery($arrAffiliation['@attributes']['node'],$strUsername,$strPassword);
        // if(!$nodeTypeResponse['error']){
        //   $nodeType = $nodeTypeResponse['output']['meta']['@attributes']['type'];
        //   if($arrAffiliation['@attributes']['affiliation'] == 'publisher'){
        //     $arrDevices['publisher'][$arrAffiliation['@attributes']['node']] = array(
        //     'id'=>$arrAffiliation['@attributes']['node'],
        //     'type'=>$nodeType,
        //     'name'=>$nodeTypeResponse['output']['meta']['@attributes']['name']
        //     );
        //   }
        //   if($arrAffiliation['@attributes']['affiliation'] == 'owner'){
        //     $arrDevices['owner'][$arrAffiliation['@attributes']['node']] = array(
        //     'id'=>$arrAffiliation['@attributes']['node'],
        //     'type'=>$nodeType,
        //     'name'=>$nodeTypeResponse['output']['meta']['@attributes']['name']
        //     );
        //   }        
        // }
      }
    }
    $return = $this->getSubscriptions($strUsername,$strPassword);
    if(!$return['error']){
      // foreach($return['subscriptions'] as $nodeKey=>&$node){
      //   $response = Mio::nodeMetaQuery($node['id'],$strUsername,$strPassword);
      //   if(!$response['error']){
      //     $arrNodeData = $response['output']['meta']['@attributes'];
      //     $node['type'] = $arrNodeData['type'];
      //     $node['name'] = $arrNodeData['name'];
      //   }else{
      //     unset($return['subscriptions'][$nodeKey]);
      //   }
      // }
      $arrDevices['subscribe'] = $return['subscriptions'];
    }    
    return array('error'=>false,'devices'=>$arrDevices);
  }
  /**
   * Get devices subscribe to a user 
   * @param $strUsername user username
   * @param $strPassword user password
   * @return error bool variable and text message
   */
  public function getSubscriptions($strUsername,$strPassword){
    $return = Mio::subscriptionsQuery(null,$strUsername,$strPassword);
    return $return;
  }
  /**
   * Uses lib mio to get the folders of an user
   * @param string $strUsername user username whose folders we want to retrieve
   * @return array containing the folders of the user or error on failure
   */
  public function getFolders($strUsername){
    $return = Mio::getFolders($strUsername);
    return $return;
  }

  /**
   * Creates a token using the username and current timestamp, 
   * then calls mio tool to save the token to the xmpp
   * @param  string $strUsername username
   * @param  bool $boolDelete should it delete the recovery code
   * @return array with error value and message, it just forwars what the tool returns.
   */
  public static function setRecoveryCode($strUsername,$boolDelete = false){
    if($boolDelete){
      return Mio::setRecoveryCode($strUsername,'');
    }
    $strRecoveryCode = md5($strUsername.time());
    return Mio::setRecoveryCode($strUsername,$strRecoveryCode);
  }
  /**
   * Gets existing token for given username
   * @param  string $strUsername username
   * @return array with error value and message, it just forwars what the tool returns.
   */
  public static function getRecoveryCode($strUsername){
    return Mio::getRecoveryCode($strUsername);
  }

  /**
   * Checks if given password matches requirements
   * @todo  what are this requirements, and validate them :P
   * @todo  implement on user edit, user create and user recover password
   * @param  string $password password to check
   * @return array with error and message;
   */
  public static function validPassword($strPassword){
    return array('error'=>false,'message'=>'Password is valid');
  }

  /**
   * Given an username and password, talks to the xmpp to change the password
   * @param  string $strUsername username to set the new password
   * @param  string $strPassword the new password
   * @return array with error and message
   */
  public static function changePassword($strUsername,$strPassword){
    return Mio::setUserPassword($strUsername,$strPassword);
  }
}