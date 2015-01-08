/**
 * Alert Handling module
 */
(function(){
  var app = angular.module('alert-handler',['ui.bootstrap']);
  /**
   * Controller for managing device actions
   * @param  object $scope    scope of the controller
   * @param  factory User     User factory instance
   * @param  service $http    angulars $http service
   * @param  service $state   ui router state service
   */
  app.controller('AlertCtrl',function($scope,Alert){
    $scope.Alert = Alert;
  });
  app.factory('Alert',function($timeout){
    var Alert = {};
    Alert.alerts = [];
    Alert.show = false;
    /**
     * Opens alert box, sets type msg and show to true.
     * @param  str type type of alert (view bootstrap classes)
     * @param  str msg  message to show user
     */
    Alert.open = function(type,msg){
      Alert.alerts.push({type:type,msg:msg});
      Alert.show = true;
    };
    /**
     * closes alert box, sets everything back to default.
     */
    Alert.close = function(){
      Alert.alerts = [];
      Alert.show = false;
    };
    return Alert;
  })
})();