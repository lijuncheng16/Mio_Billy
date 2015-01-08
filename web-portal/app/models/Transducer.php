<?php
class Transducer {
  /**
   * Calls mio tool to execute a command
   * @param  string $strUsername 
   * @param  string $strPassword 
   * @return mixed array with success or error and message.
   */
  public function actuate($intValue,$strUsername,$strPassword){
    return Mio::actuate($this->device,$this->name,$intValue,$strUsername,$strPassword);
  }
  /**
   * Gets historical data of a transducer
   * @param  string $strDeviceName device from which retrieve data
   * @param  string $strTransducer transducer identifier
   * @param  string $strStart      start date in the Y-m-d format
   * @param  string $strEnd        end date in the Y-m-d format
   * @return array containing historical data of device
   */
  public function getTimeseriesData($strStart,$strEnd,$strUsername,$strPassword){
    $return = Mio::getTransducerTimeseries($this->device,$this->name,$strStart,$strEnd,$strUsername,$strPassword);
    return $return;
  }
  
}