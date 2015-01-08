(function(){
  /**
   * Show the schedule exception form
   */
  var app=angular.module('exception-form-directive',[]);
  app.directive('scheduleExceptionForm',function(){
    return {
      restrict:'A',
      scope:{
        schedule:'=',
        transducers:'=',
        submit:'&onSubmit',
        showSubmit:'='
      },
      templateUrl:'/angular-app/partials/schedule-form.html',
      link:function(scope,element,attrs){
        scope.today=new Date();
        scope.open=function($event){
          $event.preventDefault();
          $event.stopPropagation();
          scope.opened=true;
        };
      }

      
    };
  });
})();