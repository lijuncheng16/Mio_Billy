<?php

class UserController extends BaseController {

  /**
   * login
   * @return JSON user info, or error array
   */
  public function login() {
    $arrUser = Input::all();
    $user = new User;
    $user->username = $arrUser['username'];
    $user->password = $arrUser['password'];
    $response = $user->login();
    if($response['error']){
      return Response::json($response);
    }
    $response['user']['username'] = $user->username;
    $response['user']['password'] = $user->password;
    Session::put('user',$response['user']);
    return Response::json($response);
  }

  public function logout(){
    Session::forget('user');
    return Response::json('deleted');
  }


  /**
   * Get list of registered users
   * @return JSON containing the users data
   */
  public function index(){
    $user = new User;
    $user->username = $this->user['username'];
    $user->password = $this->user['password'];
    $response = $user->index();
    return Response::json($response);
  }

  /**
   * Get the data of a user given its ID
   * @param int $userId ID of the user to retrieve 
   * @return JSON containing the user data
   */
  public function displayUser($strUsername){
    $user = new User;
    $response = $user->displayUser($strUsername);
    return Response::json($response);
  }
  
  /**
   * Creates a new user using form data
   * @return json containig a message of either success or error
   */
  public function createUser(){
    $arrData = Input::all(); 
    try {
      $user = User::fromArrayAll($arrData);         
    } catch (Exception $e) {
       return Response::json(array('error'=>true,'message'=>$e->getMessage()));  
    }   
    $response = $user->createUser();
    return Response::json($response);
  }
  
  /**
   * Edits the data of an user
   * @param array $arrData user data to save
   * @return json containig a message of either success or error 
   */
  public function editUser($strUsername){ 
    $arrData = Input::all();   
    $arrData['username'] = $strUsername;   
    try {
      $user = User::fromArray($arrData);         
    } catch (Exception $e) {
       return Response::json(array('error'=>true,'message'=>$e->getMessage()));  
    }   
    $response = $user->edit();
    return Response::json($response);
  }

  /**
   * Deletes an user
   * @param string $strUsername  user username
   * @return json containig a message of either success or error 
   */
  public function deleteUser($strUsername){
    $user = new User();
    $response = $user->deleteUser($strUsername);
    return Response::json($response);
  }

  /**
   * gets the permitted devices of an user
   * @param string $strUsername user username to  get devices
   * @return JSON list of permited devices
   */
  public function getPermittedDevices(){
    $user = new User;
    $user->username = $this->user['username'];
    $response = $user->getPermittedDevices($this->user['username'],$this->user['password']);
    return Response::json($response);
  }
  
  /**
   * Gets the folders of an user
   * @param string $strUsername user username whose folders we want to retrieve
   * @return json containing the folders of the user
   */
  public function getFolders($strUsername){
    $user = new User();
    $user->username = $this->user['username'];
    $user->password = $this->user['password'];
    $response = $user->getFolders($strUsername);
    return Response::json($response);
  }
  
  /**
   * Creates a recovery token for a user, then sends an email so that the user can reset their password
   * @param  string $strUsername username to create the token to
   * @return array with error value and message.
   */
  public function passwordRecovery($strUsername){
    $arrUser = Mio::displayUser($strUsername);
    if($arrUser['error']){
      return Response::json($arrUser);
    }
    $arrToken = User::setRecoveryCode($strUsername);
    if($arrToken['error']){
      return Response::json($arrToken);
    }
    $strEmail = $arrUser['user']['email'];
    try{
      Mail::send('emails.password_recovery',array('user'=>$arrUser['user'],'token'=>$arrToken['token']),function($message) use ($strEmail){
        $message->to($strEmail)->subject('Mio password recovery');
      });
    }catch(Exception $e){
      return Response::json(array('error'=>true,'message'=>$e->getMessage()));
    }
    return Response::json(array('error'=>false,'message'=>'An email was sent with the recovery link to '. $strEmail));
  }
  /**
   * Changes the user password if the passed token is valid.
   * @param string $strUsername username who's password must be reset
   * @return array with error value and message.
   * @todo  full implementation, this is just a stub so that jairo gets a response
   */
  public function setPassword($strUsername){
    $arrInput = Input::all();
    $strPassword = $arrInput['password'];
    $strPasswordConfirm = $arrInput['password_confirm'];
    $strInputToken = $arrInput['token'];
    
    if(is_null($strInputToken)){
      return Response::json(array('error'=>true,'message'=>'No token given'));
    }
    $arrUserRecovery = User::getRecoveryCode($strUsername);
    if($arrUserRecovery['error']){
      return Response::json($arrUserRecovery);
    }
    if($strInputToken != $arrUserRecovery['token']){
      return Response::json(array('error'=>true, 'message'=>'Token is incorrect')); 
    }
    if(is_null($strPassword) || is_null($strPasswordConfirm)){
      return Response::json(array('error'=>true, 'message'=>'Missing passwords'));
    }
    if($strPassword != $strPasswordConfirm){
      return Response::json(array('error'=>true, 'message'=>'Passwords do not match'));
    }
    $arrValidPassword = User::validPassword($strPassword);
    if($arrValidPassword['error']){
      return Response::json($arrValidPassword);
    }
    $arrPasswordChange = User::changePassword($strUsername,$strPassword);
    if($arrPasswordChange['error']){
      return Response::json($arrPasswordChange);
    }
    $arrRemoveToken = User::setRecoveryCode($strUsername,true); //delete current token for the user
    if($arrRemoveToken['error']){
      return Response::json(array('error'=>false, 'message'=>'Password changed successfully, but couldn\'t remove the old token'));
    }
    return Response::json(array('error'=>false, 'message'=>'Password changed successfully'));
  }
}