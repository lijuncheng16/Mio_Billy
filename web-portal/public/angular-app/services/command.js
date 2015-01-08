(function(){
  var app=angular.module('command-service',['transducer-service']);

  app.service('Command',function(Transducer){
    this.id=null;
    this.transducer=Transducer;
    this.value=0;
    this.run = function(){
      return this.transducer.actuate(this.value);
    };
  });
})();