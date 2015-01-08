<?php namespace LibMio\Facades;
use Illuminate\Support\Facades\Facade;

class MioFacade extends Facade {
  /**
   * Returns mio library, for access as a static class
   * @return string name of underlying class
   */
  protected static function getFacadeAccessor(){ return 'mio';}
}