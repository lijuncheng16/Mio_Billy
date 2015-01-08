<?php 
Class Monitor {
  public $id;
  public $name;
  private static $xmlns = 'http://mortar.io/monitor/';
  /**
   * List of transducers, has name and device it belongs to
   * @var array
   */
  public $transducers;
  
  /**
   * Given a monitor id, fetches the transducers using Mio
   * @param  string $strId       id of the monitor
   * @param  string $strUsername owner of the monitor
   * @param  string $strPassword user password
   * @return monitor instance of Monitor
   * @throws Exception If there was an error retrieving the xml from xmpp
   */
  public static function fromId($strId,$strUsername,$strPassword){
    $strMonitorXml = Mio::getMonitor(self::$xmlns.$strId,$strUsername,$strPassword);
    if($strMonitorXml['error']){
      throw new Exception($strMonitorXml['message']);
    }
    return self::fromXml($strMonitorXml['monitor']);
  }
  /**
   * Creates an instance from an array representation of a monitor
   * @param  array $arrMonitor representation of monitor
   * @return monitor instance of Monitor
   */
  public static function fromArray($arrMonitor){
    $monitor = new self();
    if(isset($arrMonitor['name']) && !empty($arrMonitor['name'])){
      $monitor->name = $arrMonitor['name'];
    }else{
      $monitor->name = 'Monitor '.time();
    }
    $monitor->id = (isset($arrMonitor['id']) && !empty($arrMonitor['id'])) ? $arrMonitor['id'] : UUID::generate(4);
    if(isset($arrMonitor['transducers']) && !empty($arrMonitor['transducers'])){
      $monitor->transducers = isset($arrMonitor['transducers'][0]) ? $arrMonitor['transducers'] : array($arrMonitor['transducers']);
    }else{
      $monitor->transducers = array();
    }
    return $monitor;
  }
  public static function index($strUsername,$strPassword){
    return Mio::listMonitors($strUsername,$strPassword);
  }
  /**
   * Calls Mio library, to save the monitor
   * @param  string $strUsername username of the current user
   * @param  string $strPassword password of the current user
   * @param  string $isNameUpdate boolean, tells its an update, and the name needs to be changed in the list of monitors
   * @return array with error or success message
   * @throws Excetion If could not save
   * @todo  save the monitor id in the list of monitors of the user.
   */
  public function saveMonitor($strUsername,$strPassword,$isNameUpdate = false){
    $response = Mio::saveMonitor($this->id,$this->toXml(),$strUsername,$strPassword);
    if($response['error']){
      throw new Exception($response['message']);
    }
    $arrMonitors = Mio::listMonitors($strUsername,$strPassword);
    if($arrMonitors['error']){
      throw new Exception('Could not get monitors list to update it');
    }
    if($isNameUpdate){
      $isUpdated = false;
      foreach($arrMonitors['monitors'] as &$monitor){
        if($this->id == $monitor['id']){
          $monitor['name'] = $this->name;
          $isUpdated = true;
          break;
        }
      }
      if($isUpdated){
        $response = Mio::saveListMonitors($arrMonitors['monitors'],$strUsername,$strPassword);
        if($response['error']){
          throw new Exception($response['message']);
        }
        return;
      }
    }else{
      $arrMonitors['monitors'][] = array(
          'id'=>$this->id,
          'name'=>$this->name
        );
      $response = Mio::saveListMonitors($arrMonitors['monitors'],$strUsername,$strPassword);
      if($response['error']){
        throw new Exception($response['message']);
      }
    }
  }
  public static function delete($strId,$strUsername,$strPassword){
    $arrMonitors = Mio::listMonitors($strUsername,$strPassword);
    if($arrMonitors['error']){
      return $arrMonitors;
    }
    $isDeleted = false;
    foreach($arrMonitors['monitors'] as $key=>$monitor){
      if($strId == $monitor['id']){
        unset($arrMonitors['monitors'][$key]);
        $isDeleted = true;
        // break;
      }
    }
    if($isDeleted){
      return Mio::saveListMonitors($arrMonitors['monitors'],$strUsername,$strPassword); 
    }
    return array('error'=>true,'message'=>'Monitor does not exist');
  }
  /**
   * Creates an instance from an xml string
   * @param  string $strXml xml string representation of a monitor
   * @return monitor instance of Monitor
   * @throws  Exception If xmlobj is empty;
   */
  private static function fromXml($strXml){
    $objXmlMonitor = simplexml_load_string($strXml);
    $arrMonitor = json_decode(json_encode($objXmlMonitor),true);
    if(empty($arrMonitor)){
      throw new Exception('Monitor does not exist');
    }
    return self::fromArray($arrMonitor);
  }
  /**
   * Generates XML string from monitor object
   * @return string xml representation of monitor
   */
  public function toXml(){
    $monitor = new SimpleXMLElement('<monitor />');
    $monitor->addAttribute('xmlns',self::$xmlns.$this->id);
    $monitor->addChild('id',$this->id);
    $monitor->addChild('name',$this->name);
    foreach($this->transducers as $transducer){
      $t = $monitor->addChild('transducers');
      $t->addChild('id',$transducer['id']);
      $t->addChild('name',$transducer['name']);
      $t->addChild('device',$transducer['device']);
    }
    return trim(str_replace('"', "'",str_replace('<?xml version="1.0"?>', '', $monitor->asXML()))); // we don't need the xml version
  }
}