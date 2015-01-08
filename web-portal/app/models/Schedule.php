<?php 
Class Schedule {
  public $device;
  public $id;
  public $time;
  public $info;
  public $tName;
  public $tValue;
  public $rFreq;
  public $rInt;
  public $rCount;
  public $rUntil;
  public $rByMonth = array();
  public $rByDay = array();
  public $exdate = array();
  
  /**
   * Calls mio tools meta data tool, extracts schedules from it.
   * @param  string $strUsername [description]
   * @param  string $strPassword [description]
   * @return mixed array with error, on succes array with schedules
   */
  public static function index($strDevice,$strUsername,$strPassword){
    $response = Mio::scheduleQuery($strDevice,$strUsername,$strPassword);
    return $response;
  }  
  
  /**
   * Creates schedule instance from array representation
   * @param  string $strDevice   device name
   * @param  array  $arrSchedule array representation of the schedule
   * @return Schedule instance of the schedule
   */
  public static function fromArray($strDevice,$arrSchedule){
    $schedule = new self();
    $schedule->device = $strDevice;

    if(isset($arrSchedule['id'])){
      $schedule->id = $arrSchedule['id'];
    }
    if(!isset($arrSchedule['time']) || empty($arrSchedule['time'])){
      if(!isset($arrSchedule['id']) && empty($arrSchedule['id'])){
        throw new Exception('No start date provided');
      }
    }else{
      $schedule->time = date('c',strtotime($arrSchedule['time']));
    }
    if(isset($arrSchedule['info']) && !empty($arrSchedule['info'])){
      $schedule->info = $arrSchedule['info'];
    }else{
      $schedule->info = null;
    }
    if(!isset($arrSchedule['t_name']) || empty($arrSchedule['t_name'])){
      if(!isset($arrSchedule['id']) && empty($arrSchedule['id'])){
        throw new Exception('No transducer provided');
      }
    }else{
      $schedule->tName = $arrSchedule['t_name'];
    }
    if(!isset($arrSchedule['t_value'])){
      if(!isset($arrSchedule['id']) && empty($arrSchedule['id'])){
        throw new Exception('No transducer provided');
      }
    }else{
      $schedule->tValue = $arrSchedule['t_value'];
    }
    if(!isset($arrSchedule['freq']) || empty($arrSchedule['freq'])){
      if(!isset($arrSchedule['id']) && empty($arrSchedule['id'])){
        $schedule->rFreq = DAILY;
      }
    }else{
      $schedule->rFreq = $arrSchedule['freq'];
    }
    if(isset($arrSchedule['interval']) && !empty($arrSchedule['interval'])){
      $schedule->rInt = $arrSchedule['interval'];
    }
    if(isset($arrSchedule['count']) && !empty($arrSchedule['count'])){
      $schedule->rCount = $arrSchedule['count'];
    }
    if(isset($arrSchedule['until']) && !empty($arrSchedule['until'])){
      $schedule->rUntil = date('c',strtotime($arrSchedule['until']));
    }
    /**
     * accepted values are : 1-12
     * @todo validate this.
     */
    if(isset($arrSchedule['bymonth']) && !empty($arrSchedule['bymonth'])){
      // $validator = Validator::make(array('bymonth'=>$arrSchedule['bymonth']),array('bymonth'=>array('array','between:1,12')));
      // pr($validator->failed());
      // die('month');
      $schedule->rByMonth = $arrSchedule['bymonth'];
    }
    /**
     * accepted values are : SU,MO,TU,WE,TH,FR,SA 
     * @todo validate this.
     */
    if(isset($arrSchedule['byday']) && !empty($arrSchedule['byday'])){
      // $validator = Validator::make(array('byday'=>$arrSchedule['byday']),array('byday'=>array('array','in:LA')));
      // pr($validator->messages());
      // die('day');
      $schedule->rByDay = $arrSchedule['byday'];
    }
    if(isset($arrSchedule['exdate']) && !empty($arrSchedule['exdate'])){
      $schedule->exdate = is_array($arrSchedule['exdate']) ? $arrSchedule['exdate'] : array($arrSchedule['exdate']) ;
    }
    return $schedule;
  }
  private function toArray(){
    $arrSchedule = array();
    $arrSchedule['id'] = $this->id;
    $arrSchedule['time'] = $this->time;
    $arrSchedule['info'] = $this->info;
    $arrSchedule['t_name'] = $this->tName;
    $arrSchedule['t_value'] = $this->tValue;
    $arrSchedule['r_freq'] = $this->rFreq;
    $arrSchedule['r_int'] = $this->rInt;
    $arrSchedule['r_count'] = $this->rCount;
    $arrSchedule['r_until'] = $this->rUntil;
    $arrSchedule['r_bymonth'] = $this->rByMonth;
    $arrSchedule['r_byday'] = $this->rByDay;
    $arrSchedule['r_exdate'] = $this->exdate;
    return $arrSchedule;
  }
  /**
   * Creates a schedule for a device
   * @param  string $strUsername
   * @param  string $strPassword
   * @return array with success or error message
   * @todo call the mio tool, to create the schedule
   */
  public function save($strUsername,$strPassword){
    $bolErr = Mio::saveSchedule($this->device,$this->toArray(),$strUsername,$strPassword);
    if($bolErr){
      return array('error'=>true,'message'=>'Could not save schedule');
    }  
    return array('error'=>false,'message'=>'Schedule save');
  }
  /**
   * Deletes a schedule of a device
   * @param  string $strUsername
   * @param  string $strPassword
   * @return array with success or error message
   */
  public function delete($strUsername,$strPassword){
    return Mio::removeSchedule($this->device,$this->id,$strUsername,$strPassword);
  }
  /**
   * Helper function, takes a date and formats it for the Icalendar
   * @param  string $date regular date
   * @return string Icalendar date
   */
  private static function dateToIcalDate($date){
    return date('Ymd',strtotime($date)).'T'.date('His',strtotime($date));
  }
}