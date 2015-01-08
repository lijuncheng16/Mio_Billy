(function(){
  var app = angular.module('transducer-service',[]);
  app.service('Transducer',function($http){
    var id=null;
    this.device_id='';
    this.name='';
    this.values=null; //multiple values in case of a ENUM
    this.value=0; //int value
    var min_value=0;
    var max_value=0;
    this.actuate=function(intValue){
     return $http.post('/api/device/'+this.device_id+'/'+this.name+'/run_command',{value:intValue});
    };
  });
})();