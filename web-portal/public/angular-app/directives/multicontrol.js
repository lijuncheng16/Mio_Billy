(function(){
  var app = angular.module('multicontrol-directive',['alert-handler']);

  app.directive('multicontrol', function(Alert){
    return {
      restrict: 'A',
      scope: true,
      templateUrl: '/angular-app/partials/multicontrol-template.html', 
      link: function(scope,element,attrs){
        var callbackFunction =attrs.callbackFunction || 'runCommands';
        scope.submit = function(){
          Alert.close();
          var commands = [];
          var inputs = [];
          var nodeList = element[0].querySelectorAll('input[type=text],input[type=number],select');
          for(var i = 0; i<nodeList.length; i++){
            inputs[i] = nodeList[i];
          }
          angular.forEach(inputs,function(el){
            var element = angular.element(el);
            var attrs = element[0].attributes;
            element.removeClass('invalid');
            if(typeof element.val()!=null && typeof element.val()!='undefined' && element.val()!='?' && element.val()!=''){
              commands.push({
                device: attrs.device.value,
                transducer: attrs.transducer.value,
                value: element.val()
              });            
            }else{
              element.addClass('invalid');
              return;
            }
          });
          if(commands.length>0){
            var commandsCopy = [];
            angular.copy(commands,commandsCopy);
            scope.standBy = true;
            scope[callbackFunction](commandsCopy);         
          }
        }
      }
    }
  });
})();