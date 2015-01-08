(function(){
  var app = angular.module('mortar-app',['ui.router','mortar-services','mortar-controllers','mortar-directives','alert-handler']);
  /**
   * Configures the apps routes
   * @param  object $stateProvider     ui-router service
   * @param  object $urlRouterProvider ui-router service
   */
  app.config(function($stateProvider,$urlRouterProvider) {
    $urlRouterProvider.otherwise('/login');
    $stateProvider.
    state('login', {
      url: '/login',
      templateUrl: '/angular-app/partials/login.html',
      data: {
        groups:[],
        isModal:false
      }
    }).
    state('device', {
      url:'/device',
      templateUrl:'/angular-app/partials/dashboard.html',
      controller:'FolderCtrl',
      abstract:true
    }).
    state('device.list',{
      url:'/list/?folder',
      templateUrl:'/angular-app/partials/devicelist.html',
      controller:'DeviceListCtrl',
      data:{
        groups:[],
        isModal:false
      }
    }).
    state('device.map',{
      url:'/map/:folder',
      templateUrl:'/angular-app/partials/device-map-list.html',
      controller:'DeviceMapCtrl',
      data:{
        groups:[],
        isModal:false
      }
    }).
    state('device.permissions',{
      url: '/permissions/:id?isFolder',
      onEnter: function($state,$modal,$stateParams){
        $modal.open({
          templateUrl: '/angular-app/partials/device-permissions.html',
          controller: 'DeviceSetPermissionsCtrl'         
        }).result.then(function(result){
          if(result){
            if(typeof $stateParams['isFolder'] != 'undefined'){
              return $state.go('device.list',{folder:$stateParams['id']});
            }
            return $state.go('device.view.detail',{id:$stateParams['id']});
          }
        },function(){
          if(typeof $stateParams['isFolder'] != 'undefined'){
            return $state.go('device.list',{folder:$stateParams['id']});
          }
          return $state.go('device.view.detail',{id:$stateParams['id']});
        });
      },
       data: {
          groups:[],
          isModal:true
      }
    }).
    state('device.view',{
      url:'/:id',
      templateUrl:'/angular-app/partials/device-view.html',
      controller:'DeviceCtrl',
      abstract:true
    }).
    state('device.view.edit',{
      url:'/edit',
      onEnter: function($state,$modal,$stateParams){
        $modal.open({
          templateUrl:'/angular-app/partials/device-modal.html',
          controller:'DeviceModalCtrl',
        }).result.then(function(result){
          if(result){
            return $state.go('device.view.detail',{id:$stateParams['id']});
          }
        },function(){
          return $state.go('device.view.detail',{id:$stateParams['id']});
        });
      },
      data:{
        groups:[],
        isModal:true
      }
    }).
    state('device.view.detail',{
      url:'',
      templateUrl:'/angular-app/partials/device-view-detail.html',
      controller:'DeviceViewCtrl',
      data:{
        groups:[],
        isModal:false
      }
    }).
    state('device.view.map',{
      url:'/map',
      templateUrl:'/angular-app/partials/device-map.html',
      controller:'DeviceDetailMapCtrl',
      data:{
        groups:[],
        isModal:false
      }
    }).
    state('device.view.transducers',{
      url:'/transducers',
      templateUrl:'/angular-app/partials/device-transducers.html',
      controller:'DeviceTransducersCtrl',
      data:{
        groups:[],
        isModal:false
      }
    }).
    state('device.view.timeseries',{
      url:'/timeseries',
      templateUrl:'/angular-app/partials/device-timeseries.html',
      controller:'DeviceTimeSeriesCtrl',
      data:{
        groups:[],
        isModal:false
      }
    }).
   state('device.view.schedule',{
      url:'/schedule',
      templateUrl:'/angular-app/partials/device-schedule.html',
      controller:'DeviceScheduleCtrl',
      data:{
        groups:[],
        isModal:false
      }
    }).
   state('device.view.schedule.modalschedule',{
      url:'/:schedule',
       onEnter:function($state,$stateParams,$modal){
        $modal.open({
          templateUrl:'/angular-app/partials/device-schedule-modal.html',
          controller:'ScheduleModalCtrl'
        }).result.then(function(result){
          if(result){
            return $state.transitionTo('device.view.schedule',{id:$stateParams['id']});
          }
        },function(){
          return $state.transitionTo('device.view.schedule',{id:$stateParams['id']});
        });
      },
      data:{
        groups:[],
        isModal:true
      }
    }).
    state('device.view.schedule.modalexception',{
      url:'/:schedule/exception',
      onEnter:function($state,$stateParams,$modal){
        $modal.open({
          templateUrl:'/angular-app/partials/device-schedule-exception-modal.html',
          controller:'ExceptionModalCtrl'
        }).result.then(function(result){
          if(result){
            return $state.transitionTo('device.view.schedule',{id:$stateParams['id']});
          }
        },function(){
          return $state.transitionTo('device.view.schedule',{id:$stateParams['id']});
        });
      },
      data:{
        groups:[],
        isModal:true
      }
    }).
    state('device.view.functions',{
      url:'/functions',
      templateUrl:'/angular-app/partials/device-functions.html',
      controller:'DeviceFunctionsCtrl',
      data:{
        groups:[],
        isModal:false
      }
    }).
    state('device.view.favorites',{
      url:'/favorites',
      onEnter:function($state,$stateParams,$modal){
        $modal.open({
          templateUrl:'/angular-app/partials/device-favorites-modal.html',
          controller:'DevFavoritesModalCtrl'
        }).result.then(function(result){
          if(result){
            return $state.go('device.view.detail',{id:$stateParams['id']});
          }
        },function(){
          return $state.go('device.view.detail',{id:$stateParams['id']});
        });
      },
      data:{
        groups:[],
        isModal:true
      }
    }).
    state('device.view.favorites.addfolder',{
      url:'/:modal',
      onEnter:function($state,$stateParams,$modal){
        $modal.open({
          templateUrl:'angular-app/partials/FolderModal.html',
          controller:'FolderCreateCtrl'
        }).result.then(function(result){
          if(result){
            return $state.go('^',{id:$stateParams['id']});
          }
        },function(){
          return $state.go('^',{id:$stateParams['id']});
        });
      },
     data: {
        groups:[],
        isModal:true
      }
    }).
    state('user', {
      url:'/user',
      templateUrl:'/angular-app/partials/user.html',
      abstract:true,
      data:{
        groups:[],
        isModal:false
      }
    }).
    state('user.list', {
      url: '',
      templateUrl: '/angular-app/partials/userList.html',
      controller: 'UserListCtrl',
      data: {
        groups:[],
        isModal:false
      }
    }).
    state('user.view', {
      url: '/:username',
      templateUrl: '/angular-app/partials/userDetail.html',
      controller: 'ViewUserCtrl',
      data: {
        groups:[],
        isModal:false
      }
    }).
    state('user.edit', {
      url: '/edit/:username',
      onEnter: function($state,$stateParams,$modal,User){
        $modal.open({
          templateUrl: 'angular-app/partials/addEditUserModal.html',
          controller: 'AddEditUserCtrl',
          resolve: {
            username: function(){
              return $stateParams.username;
            }
          }
        }).result.then(function(result){
          if(User.group==='admin'){
            return $state.go('user.list');
          }else{
            return $state.go('user.view',{username: $stateParams.username});
          }
        }, function(){
          if(User.group==='admin'){
            return $state.go('user.list');
          }else{
            return $state.go('user.view',{username: $stateParams.username});
          }
        });
      },
      data: {
        groups:[],
        isModal:true
      }
    }).
    state('user.add', {
      url: '',
      onEnter: function($state,$modal){
        $modal.open({
          templateUrl: 'angular-app/partials/addEditUserModal.html',
          controller: 'AddEditUserCtrl',
          resolve: {
            username: function(){
              return false;
            }
          }
        }).result.then(function(result){
          if(result) {
            return $state.transitionTo('user.list');
          }
        }, function(){
            return $state.transitionTo('user.list');
          }
        );
      },
      data: {
        groups:[],
        isModal:true
      }
    }).
    state('forgotPassword', {
      url: '/forgot_password',
      onEnter:function($state,$modal){
        $modal.open({
          templateUrl:'/angular-app/partials/userSendRecovery.html',
          controller: 'ChangePasswordCtrl'
        }).result.then(function(result){
          if(result){
            return $state.transitionTo('login');
          }
        },function(){
          return $state.transitionTo('login');
        });
      },
      data:{
        groups:[],
        isModal:true
      }
    }).
    state('recovery_password', {
      url: '/recovery_password/:user/:token',
      templateUrl: '/angular-app/partials/userResetPassword.html',
      controller: 'ResetPasswordCtrl',
      data: {
        groups:[],
        isModal:false
      }
    }).
    state('device.addfolder',{
      url:':device',
      onEnter:function($state,$stateParams,$modal){  
        $modal.open({
          templateUrl:'angular-app/partials/FolderModal.html',
          controller:'FolderCreateCtrl'
        }).result.then(function(result){
          if(result){
            return $state.go('device.list');
          }
        },function(){
          return $state.go('device.list');
        });
      },
     data: {
        groups:[],
        isModal:true
      }
    }).
    state('device.editFolder',{
      url:'/edit/:folders',
      onEnter:function($state,$stateParams,$modal){
        $modal.open({
          templateUrl:'angular-app/partials/FolderModal.html',
          controller:'FolderCreateCtrl'
        }).result.then(function(result){
          if(result){
            return $state.go('device.list',{folder:$stateParams['folders']});
          }
        },function(){
          return $state.go('device.list',{folder:$stateParams['folders']});
        });
      },
     data: {
        groups:[],
        isModal:true
      }
    }).
    state('monitor',{
      url: '/monitor',
      templateUrl: '/angular-app/partials/monitor.html',
      controller: 'MonitorListCtrl',
      abstract: true
    }).
    state('monitor.view',{
      url: '/:id',
      templateUrl: '/angular-app/partials/monitor-view.html',
      controller: 'MonitorViewCtrl',
      data: {
        groups:[],
        isModal:false
      } 
    }).
    state('monitor.add',{
      url:'',
      templateUrl:'/angular-app/partials/monitor-add.html',
      controller: 'MonitorAddCtrl',
      data: {
        groups:[],
        isModal:false
      }
    }).
    state('scheduletask',{
      url:'/scheduletask',
      templateUrl:'angular-app/partials/schedule-task.html',
      controller:'ScheduleTaskCtrl',
      data:{
        groups: []
      }
    }).
    state('multicontrol',{
      url:'/multi_control',
      templateUrl: 'angular-app/partials/multicontrol.html',
      controller: 'MulticontrolCtrl',
      data: {
        groups: []
      }
    })
  });
  /**
   * Add listener to the stateChangeStart, handles logged
   * in user and authorized
   * @param  obj $rootScope   scope of the whole app
   * @param  service $state   ui router state service
   * @param  factory User     user model
   */
  app.run(function($rootScope,$state, User,Alert,$location) {
    $rootScope.$on( "$stateChangeStart", function(event, next,toParams,fromState) {
      if(typeof fromState.data != 'undefined' && !fromState.data.isModal){
        Alert.close();        
      }
      if (!User.loggedIn) {
        if(next.templateUrl != "/angular-app/partials/login.html" && next.name != "forgotPassword") {
          event.preventDefault();
          // set user state data to redirect if necessary
          angular.copy(next, User.state);         
          User.state.params = toParams;

          Alert.open('warning','You need to be logged in');
          $state.go('login');
          return;
        }
      }
      // groups that shouldn't be there!
      var groups = next.data.groups || [];
      /**
       * Prevent route change, show alert,
       * You aint got cleareance!
       */
      if(User.loggedIn && groups.indexOf(User.group) != -1){
        console.log("you shouldn't be here");
        console.log(next,event);
        $state.go(fromState.name);
        return;
      }

      /**
       * When next state is monitor.view, check if user has monitors. 
       * If has monitors, display the first one; if not, redirect to monitor.add state
       */
      if(User.loggedIn && next.name=='monitor.view'){  
        if(toParams.id=='' || toParams.id==null){
          if(typeof User.monitors!='undefined'){
            var keys = Object.keys(User.monitors);
            if(keys.length>0){
              $location.path('monitor/'+keys[0]);
            }else{
              $location.path('monitor');
            }
          }else{
            $location.path('monitor');
          }
        }
      }
    });    
  });
})();