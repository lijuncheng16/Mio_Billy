<?php
class ScheduleController extends BaseController {
  
  /**
   * Lists the schedules of a device
   * @param  string $strName device name
   * @return JSON array with error, on success
   */
  public function index($strDevice){
    $schedules = Schedule::index($strDevice,$this->user['username'],$this->user['password']);
    return Response::json($schedules);
  }
  /**
   * Create a schedule for an action to be 
   * performed on a device
   * @param  string $strDevice device to add the schedule to
   * @return JSON array with error info if there was an error.
   */
  public function create($strDevice){
    $arrData = Input::except('id');
    $schedule = null;
    try{
      $schedule = Schedule::fromArray($strDevice,$arrData);
    }catch(Exception $e){
      return Response::json(array('error'=>true,'message'=>$e->getMessage()));
    }
    $response = $schedule->save($this->user['username'],$this->user['password']);
    return Response::json($response);
  }
  
  /**
   * Create a schedule for an action to be 
   * performed on a device
   * @param string $strSchedule schedule identifier
   * @param  string $strDevice device to add the schedule to
   * @return JSON array with error info if there was an error.
   */
  public function edit($strDevice,$strSchedule){
    $arrData = Input::all();
    $schedule = null;
    try{
      $arrData['id'] =$strSchedule;
      $schedule = Schedule::fromArray($strDevice,$arrData);
    }catch(Exception $e){
      return Response::json(array('error'=>true,'message'=>$e->getMessage()));
    }
    $response = $schedule->save($this->user['username'],$this->user['password']);
    return Response::json($response);
  }
  public function setExceptions($strDevice,$strSchedule){
    $arrExdate = Input::get('exdate',array());
    $arrData['exdate'] = is_array($arrExdate) ? $arrExdate : array($arrExdate);
    $schedule = null;
    try{
      $arrData['id'] =$strSchedule;
      $schedule = Schedule::fromArray($strDevice,$arrData);
    }catch(Exception $e){
      return Response::json(array('error'=>true,'message'=>$e->getMessage()));
    }
    $response = $schedule->save($this->user['username'],$this->user['password']);
    return Response::json($response);
  }

  /**
   * Delete a schedule from a device
   * @param string $strSchedule schedule identifier
   * @param  string $strDevice device to add the schedule to
   * @return JSON array with error info if there was an error.
   */
  public function delete($strDevice,$strSchedule){
    $schedule = new Schedule;
    $schedule->id = $strSchedule;
    $schedule->device = $strDevice;
    return Response::json($schedule->delete($this->user['username'],$this->user['password']));
  }
}