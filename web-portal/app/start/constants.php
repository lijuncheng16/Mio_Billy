<?php 

/**
 * simple debugging
 * @param  mixed $code something to print to the screen
 * @return void
 */
function pr($code){
  if (Config::get('app.debug')) {
    echo "<pre>";
    print_r($code);
    echo "</pre>";
  }
}
function my_exec($cmd, $input=''){
  $proc=proc_open($cmd, array(0=>array('pipe', 'r'), 1=>array('pipe', 'w'), 2=>array('pipe', 'w')), $pipes);
  fwrite($pipes[0], $input);fclose($pipes[0]);
  $stdout=stream_get_contents($pipes[1]);fclose($pipes[1]);
  $stderr=stream_get_contents($pipes[2]);fclose($pipes[2]);
  $rtn=proc_close($proc);
  return array(
    'stdout'=>$stdout,
    'stderr'=>$stderr,
    'return'=>$rtn
  );
}

// define('XMPP_HOST','sensor.andrew.cmu.edu');
define('XMPP_HOST','localhost');
define('MIO_ESCAPE','"');

//  Schedule Repeat Type
define('DAILY','DAILY');
define('WEEKLY','WEEKLY');
define('MONTHLY','MONTHLY');