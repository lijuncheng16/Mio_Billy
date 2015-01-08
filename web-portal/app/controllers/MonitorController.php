<?php
class MonitorController extends BaseController {
  
  /**
   * Gets the list of monitors of the user
   * @return [type] [description]
   */
  public function index(){
    return Response::json(Monitor::index($this->user['username'],$this->user['password']));
  }
  /**
   * Creates a monitor for the given user
   * @return array with error or success message
   */
  public function create(){
    $monitor = null;
    try{
      $monitor = Monitor::fromArray(Input::except('id'));
      $monitor->id = UUID::generate(4)->string;
      $monitor->saveMonitor($this->user['username'],$this->user['password']);
    }catch(Exception $e){
      return Response::json(array('error'=>true,'message'=>$e->getMessage()));
    }
    return Response::json(array('error'=>false,'monitor'=>$monitor));
  }
  /**
   * Returns list of transducers that make up the monitor
   * @param  string $strMonitor the id of the monitor
   * @return array with error message or monitor object
   */
  public function view($strMonitor){
    $monitor = null;
    try{
      $monitor = Monitor::fromId($strMonitor,$this->user['username'],$this->user['password']);
    }catch(Exception $e){
      return Response::json(array('error'=>true,'message'=>$e->getMessage()));
    }
    return Response::json(array('error'=>false,'monitor'=>$monitor));
  }
  /**
   * Edits a monitor, can change its name and add/remove transducers
   * @param  string $strMonitor id of the monitor
   * @return array with error message or monitor
   */
  public function edit($strMonitor){
    $monitor = null;
    $arrMonitor = Input::all();
    $isNameUpdate = false;
    try{
      $monitor = Monitor::fromId($strMonitor,$this->user['username'],$this->user['password']);
      if(isset($arrMonitor['name'])){ // change of name
        $isNameUpdate = true;
        $monitor->name = $arrMonitor['name'];
      }
      if(isset($arrMonitor['remove_transducers']) && !empty($arrMonitor['remove_transducers'])){ //transducers to remove associative array device/transducer
        foreach($monitor->transducers as $intKey => $arrTransducer){
          if(isset($arrMonitor['remove_transducers'][$arrTransducer['device']]) && in_array($arrTransducer['id'], $arrMonitor['remove_transducers'][$arrTransducer['device']])){
            unset($monitor->transducers[$intKey]);
          }
        }
      }
      if(isset($arrMonitor['transducers']) && !empty($arrMonitor['transducers'])){ //transducers to add
        $monitor->transducers = array_merge($monitor->transducers,$arrMonitor['transducers']);
      }
      $monitor->saveMonitor($this->user['username'],$this->user['password'],$isNameUpdate);
    }catch(Exception $e){
      return Response::json(array('error'=>true,'message'=>$e->getMessage()));
    }
    return Response::json(array('error'=>false,'message'=>'Monitor successfully updated.'));
  }
  public function delete($strMonitor){
    return Response::json(Monitor::delete($strMonitor,$this->user['username'],$this->user['password']));
  }
}
// $arrMonitors = $this->listMonitors($strUsername,$strPassword);
//       if($arrMonitors['error']){
//         return array('error'=>true,'message'=>'Could not retrieve monitors list','monitor'=>$strId);
//       }
//       $arrMonitors['monitor'][]=array(
//           'id'=>$objMonitor->id,
//           'name'=>$objMonitor->name,
//         );
//       $arrSaveMonitors = $this->saveListMonitors($arrMonitors['monitor'],$strUsername,$strPassword);
//       if($arrSaveMonitors['error']){
//         return array('error'=>true,'message'=>'Could not save to monitors list','monitor'=>$strId);
//       }
//       