(function(){
  /**
   * Show the form schedule
   */
  var app=angular.module('schedule-form-directive',[]);
  app.directive('scheduleForm',function(){
    return {
      restrict:'A',
      scope:true,
      templateUrl:'/angular-app/partials/schedule-form.html',
      link:function(scope,element,attrs){
        scope.today=new Date();
        scope.openPicker = {
          start: false,
          until: false
        };
        scope.open=function($event,picker){
          $event.preventDefault();
          $event.stopPropagation();
          scope.openPicker[picker]=true;
        };
      }
    };
  });
})();