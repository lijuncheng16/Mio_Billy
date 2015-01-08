/**
 * Login Controller module
 */
(function(){
  var app = angular.module('login-controller',['ui.router','mortar-services','cgBusy','ui.bootstrap','alert-handler']);
  /**
   * Controller for managing user login / logout
   * @param  object $scope    scope of the controller
   * @param  factory User     User factory instance
   * @param  service $http    angulars $http service
   * @param  service $state   ui router state service
   */
  app.controller('LoginCtrl',function($scope,User,$http,$state,Alert,Folder,Device){
    $scope.user = User;
    $scope.username = '';
    /**
     * Authenticates the user. Uses user scope, creates session
     * on error displays them on the form.
     */
    $scope.login = function(){
      $scope.loginPost = $http.post('/api/user/login',{username:$scope.user.username,password:$scope.user.password}).
        success(function(response){
          if(response.error){
            console.log('there are some errors ' + response.message);
            Alert.open('warning','Could not log you in, check your username and password and try again.');
            $scope.user.password = "";
          }else{
            $scope.user.name = response.user.name;
            $scope.user.email = response.user.email;
            $scope.user.group = response.user.group;
            $scope.userDevicesPromise = $scope.user.getPermittedDevices();
            $scope.user.getMonitors();            
            Folder.init();
            $scope.userDevicesPromise.then(function(result){
              $scope.user.loggedIn = true;
              $scope.user.saveSession();
              if(typeof $scope.user.state.name!='undefined'){
                $state.go($scope.user.state.name,$scope.user.state.params);
                return;
              }else{
                $state.go('device.list');              
              }              
            });
          }
        }).error(function(response){
          Alert.open('danger','Something wrong with the server');
        });
      };
      /**
       * Logs the user out, deletes session
       * tells server to delete session
       */
      $scope.logout = function(){
        $scope.user.deleteSession();
        $http.get('/api/user/logout');
        $state.go('login');
        Alert.open('success','Logged out');
      };       
    });
})();