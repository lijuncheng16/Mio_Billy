/**
 * Device Controller module
 */
(function(){
  var UPDATE_TRANSDUCER_INTERVAL = 20000;
  var app = angular.module('device-controller',['ui.router','mortar-services','cgBusy','ui.bootstrap','alert-handler','angularFileUpload','checklist-model','mightyDatepicker','olmap-directive','ja.qr']);
  /**
   * Controller for managing device actions
   * @param  object $scope    scope of the controller
   * @param  service $http    angulars $http service
   * @param  service $state   ui router state service
   * @param  factory User     User factory instance
   */
  app.controller('DeviceCtrl',function($scope,$http,$state,$stateParams,User,Folder,Device,Alert ){
    $scope.folder=Folder;
    $scope.promise = Device.get($stateParams['id']);
    $scope.device=null;
    $scope.devices=[];
    $scope.permittedDevices = User.permittedDevices['publisher'];
    $scope.subscribeDevices = User.permittedDevices['subscribe'];
    $scope.ownerDevices = User.permittedDevices['owner'];
    $scope.subscrbValidator = {isSubscribe:false,isUnsubscribe:false};
    $scope.user=User;
    $scope.promise.then(function(object){
      $scope.device=object;

      $scope.subscrbValidator.isSubscribe =User.isSusbscribe($scope.device.id);
      
      $scope.subscrbValidator.isUnsubscribe = User.isUnSusbscribe($scope.device.id);
      
      $scope.isOwner = typeof $scope.ownerDevices[$scope.device.id] !== 'undefined';
      $scope.isPublishOrOwner = $scope.isOwner || typeof $scope.permittedDevices[$scope.device.id] !== 'undefined';
      if(typeof $scope.device.parent != 'undefined'){
        $http.get('/api/device/'+$scope.device.parent.id).success(function(response){
          if(!response.error){
            $scope.device.parentDetail = response.device;
          }
        });
      }
    });
    
    $scope.subscribe = function (){   
      $scope.promiseSubs=$scope.device.subscribeUser();
      $scope.promiseSubs.success(function(response){
        if(response.error){
          Alert.open('warning',response.message);
          return;
        }
        User.getPermittedDevices();
        $scope.subscrbValidator.isSubscribe = false;
        $scope.subscrbValidator.isUnsubscribe = true;
        Alert.open('success',response.message);
      });
    };
    $scope.unSubscribe = function(){
      $scope.promiseUnsubs=$scope.device.unSubscribeUser();
      $scope.promiseUnsubs.success(function(response){
        if(response.error){
          Alert.open('warning',response.message);
          return;
        }
        User.getPermittedDevices();
        $scope.subscrbValidator.isSubscribe = true;
        $scope.subscrbValidator.isUnsubscribe = false;
        Alert.open('success',response.message);
      });
    };
  });
  /**
   * Constroller in charge of showing the map in the device list
   * @param  object $scope       controller scope
   * @param  object $stateParams url params
   * @param  service Device       Device service
   */
  app.controller('DeviceMapCtrl',function($scope,$stateParams,Device){
    $scope.folder = $stateParams.folder;
    $scope.device = typeof $stateParams.device != 'undefined' ? $stateParams.device : null;
  });
  /**
   * Constroller in charge of showing the map in the device detail
   * @param  object $scope       controller scope
   * @param  object $stateParams url params
   * @param  service Device       Device service
   */
  app.controller('DeviceDetailMapCtrl',function($scope,$stateParams,Device){
    $scope.$watch('device',function(){
      if($scope.device == null ){
        $scope.promise = Device.get($stateParams['id']);
        $scope.promise.then(function(object){
          $scope.device=object;
        });
      }
    });
  });
  /**
   * Controller for managing device detail view
   * @param  object $scope  
   * @param  service $http  
   * @param  factory Device 
   * @param  factory User           
   */
  app.controller('DeviceViewCtrl',function($scope,$http,$state,$stateParams,Device,User,$location,$window,Alert,Folder){
    var modalInstance;
    $scope.user=User;
    $scope.qrString = $location.$$absUrl;
    $scope.$watch('device',function(){
      if($scope.device == null ){
        $scope.promise = Device.get($stateParams['id']);
        $scope.promise.then(function(object){
          $scope.device=object;
        });
      }
    });
  });

  app.controller('DeviceTransducersCtrl',function($scope,$http,User,$interval){
    $scope.user=User;    
    $scope.$watch('device',function(){
      if($scope.device != null && angular.isUndefined($scope.device.transducers)){
        $scope.device.getTransducers();
      }
    });
    
    // update transducers value each 3 minutes  
    var i = $interval(function(){
      if($scope.device!=null && angular.isDefined($scope.device.transducers)){
        $scope.device.updateTransducers();
      }
    },UPDATE_TRANSDUCER_INTERVAL);

    // on scope change (destroy) cancel interval
    $scope.$on('$destroy', function(){ 
      $interval.cancel(i); 
    }); 
  });
  /**
   * Device Schedule Controller to manage the list of Schedule
   * @param  service $scope [description]
   * @param  service $http  [description]
   * @param  service Device [description]
   * @param  factory User   [description]
   */
  app.controller('DeviceScheduleCtrl',function($scope,$http,$interval,$window,User,Schedule){
    $scope.user=User;
    $scope.schedule=Schedule;
    $scope.$watch('device',function(){
      if($scope.device != null){
        $scope.getDeviceSchedules();
      }
    }); 
    /**
     * Get schedule from device 
     */
    $scope.getDeviceSchedules=function(){
      $scope.schedule.getSchedules($scope.device.id);
    };
    $scope.orderByVal = function (x){
      return x;
    }
    // $scope.intervalPromise=$interval('getDeviceSchedule',(10*1000)*60);
  });
   /**
   * Device Modal Controller to manage the edit a create device
   * @param  service $scope [description]
   * @param  service $modalInstance Instance of the modal
   * @param  service $http  [description]
   * @param  service Device [description]
   * @param  factory User   [description]
   */
  app.controller('DeviceModalCtrl',function($scope,$modalInstance,$state,$stateParams,$http,$upload,$window,$modal,Device,Folder,Alert){
    $scope.parentDevice ={};
    $scope.showAutomap = false;
    $scope.$watch('device',function(){
      if($scope.device == null){
        $scope.loadDevice = Device.get($stateParams['id']);
        $scope.device=null;
        
        $scope.loadDevice.then(function(object){
          $scope.device = object;
          if(typeof $scope.device.location === 'undefined'){
            $scope.showAutomap = true;
            $scope.device.location = {};
            $scope.device.location.lon = 0;
            $scope.device.location.lat = 0;
            $scope.device.location.street = '';
            $scope.device.location.building = '';
            $scope.device.location.floor = '';
            $scope.device.location.room = '';
          }
          if(typeof $scope.device.parent !== 'undefined'){
            $scope.parentDevice.selectNodeLabel(Folder.references[$scope.device.parent.id]);
          }
          if(typeof $scope.device.imageUrl==='undefined'){
            $scope.device.imageUrl = '';
          }
        });
      }
    });
    /**
     * Gets a list of possible devices in the same location as the current device.
     * opens a modal with the results.
     * @author Ignacio Cano <i.cano@gbh.com.do>
     * 
     */
    $scope.getPossibles = function(){
      $scope.loadingDevices = $http.get('/api/device/'+$scope.device.id+'/automap').success(function(response){
        if(response.error){
          Alert.open('warning','Could not get devices to automap');
          return;
        }
        if(response.length <= 0){
          Alert.open('warning','There are no devices to automap');
          return;
        }
         modalInstance = $modal.open({
          templateUrl: 'automap.html',
          scope:$scope,
          resolve: {
            devices: function () {
              return response.devices;
            }
          },
          controller: function($scope,$modalInstance,devices){
            $scope.devices = devices;
            /**
             * Copies the location of a given device
             * @author Ignacio Cano <i.cano@gbh.com.do>
             * @param string copyId id of the device to copy
             */
            $scope.copy = function(copyDevice){
              $scope.$parent.device.location = copyDevice.location;
              if(typeof copyDevice.parent != 'undefined'){
                $scope.$parent.selectedFolder = copyDevice.parent;
                if(typeof Folder.references[$scope.$parent.selectedFolder.id] == 'undefined'){
                  $scope.$parent.selectedFolder.children = [];
                  $scope.$parent.selectedFolder.type = 'location';
                  Folder.references[$scope.$parent.selectedFolder.id] = Folder.constructFolder($scope.$parent.selectedFolder);
                }
                $scope.parentDevice.selectNodeLabel(Folder.references[$scope.$parent.selectedFolder.id]);
              }
              $modalInstance.close(true);
            };
            /**
             * Closes the modal instance if the user pressed cancel
             */
            $scope.cancel = function () {
              $modalInstance.dismiss('cancel');
            };
          }
        });
      });
    };
    $scope.setDeviceLocation = function(lon,lat){
      $scope.device.location.lat = lat;
      $scope.device.location.lon = lon;
    }
    $scope.onFileSelect = function ($files){
      $scope.device.image = $files[0];
    }
    $scope.selectDevice=function(device){

    };
    $scope.selectFolder=function(folder){
      $scope.selectedFolder = folder;
    };
    $scope.isNotSelect=function(){
      return typeof $scope.selectedFolder === 'undefined';
    };
    $scope.submitDevice = function(){
      var url = '/api/device/'+$scope.device.id;
      var data = {
        image_url: $scope.device.imageUrl,
        name:$scope.device.name,
        info:$scope.device.info,
        lon:$scope.device.location.lon,
        lat:$scope.device.location.lat,
        street:$scope.device.location.street,
        building:$scope.device.location.building,
        floor:$scope.device.location.floor,
        room:$scope.device.location.room,
        old_parent: typeof $scope.device.parent !='undefined' ? $scope.device.parent.id : null,
        new_parent:$scope.selectedFolder.id
      };
      var success =function(response){
        if(response.error){
          $window.alert(response.message);
          return;
        }
        Alert.open('success',response.message);
        

        if( typeof $scope.device.parent !='undefined' && $scope.device.parent.id != $scope.selectedFolder.id){
          Folder.references[$scope.device.parent.id].getChildren();
          $scope.selectedFolder.getChildren();
          $scope.device.parent = $scope.selectedFolder;
        }else {
          if(typeof $scope.device.parent =='undefined'){
            $scope.selectedFolder.getChildren();
            $scope.device.parent = $scope.selectedFolder;
          }
        }
        $modalInstance.close(true);
      };
      var error = function(response){
        Alert.open('danger',response);
      };

      if(typeof $scope.device.image != 'undefined'){
        $scope.sendDevice = $upload.upload({
          url:url,
          data:data,
          file:$scope.device.image,
          fileFormDataName:'device_image'
        }).success(success).error(error);
      }else{        
        $scope.sendDevice = $http.post(url,data).success(success).error(error);
      }
    };
    $scope.cancel = function(){
      $modalInstance.dismiss();
    };
  });
  /**
   * [description]
   * @param  service $scope         [description]
   * @param  service $modalInstance [description]
   * @param  service $state         [description]
   * @param  service $stateParams   [description]
   * @param  {[type]} Device         [description]
   * @param  service Schedule       [description]
   * @param  factory User           [description]
   * @return {[type]}                [description]
   */
  app.controller('ScheduleModalCtrl',function($scope,$modalInstance,$state,$stateParams,Schedule,User,Device){
    $scope.$watch('device',function(){
      if($scope.device == null){
        $scope.schuduleLoadDev = Device.get($stateParams['id']);
        $scope.device=null;
        
        $scope.schuduleLoadDev.then(function(object){
          
          $scope.device=object;
          if(typeof $scope.schedule.t_name !== 'undefined'){ 
            for(var trans in $scope.device.transducers){
              if($scope.device.transducers[trans].name == $scope.schedule.t_name ){
                $scope.transducer.instance = $scope.device.transducers[trans];
                if(typeof $scope.transducer.instance.enum !== 'undefined'){
                  for(var index in $scope.transducer.instance.enum){
                    if($scope.transducer.instance.enum[index].value == $scope.schedule.t_value){
                      console.log('how many times?');
                      $scope.schedule.t_value = $scope.transducer.instance.enum[index].value;
                    }
                  }//end for
                }else if($scope.schedule.t_value){
                  $scope.validator.t_value=$scope.schedule.t_value * 1;
                }//end else
              }
            }
          }
        });
      }
    }); 
    $scope.validator = {};
    $scope.validator.ends='never';
    $scope.transducer={};
    $scope.transducer.instance={};
    $scope.calendar = {};
    $scope.calendar.dayOfWeek ={SU:true,MO:true,TU:true,WE:true,TH:true,FR:true,SA:true};
    $scope.calendar.months = {'1':true,'2':true,'3':true,'4':true,'5':true,'6':true,'7':true,'8':true,'9':true,'10':true,'11':true,'12':true};
    if($stateParams['schedule'] === ''){
      $scope.isUpdate = false;
      $scope.schedule = {
        freq:'DAILY',
        interval:1
      };
    }else{
      $scope.isUpdate = true;
      $scope.schedule = Schedule.get($stateParams['schedule']);
      $scope.schedule.interval *=1;
      if(typeof $scope.schedule.byday !== 'undefined' && $scope.schedule.byday !== null){
        $scope.calendar.dayOfWeek ={SU:false,MO:false,TU:false,WE:false,TH:false,FR:false,SA:false};
        for(var day in $scope.schedule.byday){
          $scope.calendar.dayOfWeek[$scope.schedule.byday[day]]=true; 
        }
      }
      if(typeof $scope.schedule.bymonth !== 'undefined' && $scope.schedule.bymonth !== null){
        $scope.calendar.months = {'1':false,'2':false,'3':false,'4':false,'5':false,'6':false,'7':false,'8':false,'9':false,'10':false,'11':false,'12':false};
        for(var month in $scope.schedule.bymonth){
          $scope.calendar.months[$scope.schedule.bymonth[month]]=true; 
        }
      }
      if(typeof $scope.schedule.count !== 'undefined'){
        $scope.validator.ends = 'count';
      }
      if(typeof $scope.schedule.until !== 'undefined'){
        $scope.validator.ends = 'until';
      }
    }

    $scope.submit=function(){
      if(angular.isDefined($scope.transducer.instance)){
        $scope.schedule.t_name=$scope.transducer.instance.name;
        if(angular.isDefined($scope.transducer.instance.enum)){
          $scope.schedule.t_value=$scope.schedule.t_value;
        }else{
          $scope.schedule.t_value = $scope.validator.t_value;
        }
      }
      console.log($scope.schedule);
      if($scope.schedule.freq == 'WEEKLY'){
        $scope.schedule.byday =[];
        for(var valueDay in $scope.calendar.dayOfWeek){
          if($scope.calendar.dayOfWeek[valueDay]){
            $scope.schedule.byday.push(valueDay);
          }
        }
      }else{
        $scope.schedule.byday =[];
      }

      if($scope.schedule.freq == 'MONTHLY'){
        $scope.schedule.bymonth =[];
        for(var valueMonth in $scope.calendar.months){
          if($scope.calendar.months[valueMonth]){
            $scope.schedule.bymonth.push(valueMonth);
          }
        }
      }else{
        $scope.schedule.bymonth =[];
      }
      var isSuccess=false;
      isSuccess=Schedule.save($scope.schedule);
      
      if(isSuccess){
        $modalInstance.close(true);
      }
      
    };
    /**
     * Close the create folder modal 
     */
    $scope.cancel = function(){
      $modalInstance.dismiss();
    };
    
  });
  /**
   * Controller to manange the creation of exception
   * @param  service $scope   current scope of the controller
   * @param  service $http    http service to mange the requesto to backend
   * @param  service Schedule service object to manage schedule crud
   */
  app.controller('ExceptionModalCtrl',function($scope,$http,$state,$stateParams,$modalInstance,$window,Alert,Schedule){
    $scope.schedule=Schedule.get($stateParams['schedule']);
    $scope.exdates = [];
    if(typeof $scope.schedule.exdate != 'undefined'){
      for(var i in $scope.schedule.exdate){
        $scope.exdates.push(moment($scope.schedule.exdate[i]));
      }
    }
    $scope.options = {
      start: moment(), 
      mode: "multiple", 
      after: moment()
    };
    /**
     * Close the create folder modal 
     */
    $scope.cancel = function () {     
      $modalInstance.dismiss();
    };
    /**
     * Submit the exception form
     */
    $scope.submit = function (dates){

      /**
       * so... lets not send the moment object, and just what we need.
       * populate new array with exdates in yyyy-mm-dd format.
       */
      var exdates = [];
      for(var i in dates){
        exdates.push(dates[i].format('YYYY-MM-DD'));
      }
      $scope.loading = $http.post('/api/schedule/'+$stateParams['id']+'/'+$stateParams['schedule']+'/exceptions',{exdate:exdates})
        .success(function(response){
          if(response.error){
            $window.alert(response.message);
            return;
          }
          $scope.schedule.exdate = exdates;
          $modalInstance.close(true);
          Alert.open('success','Exceptions saved');
        });
    };
  });
  /**
   * Controller for handling timeseries data of the device,
   * gets the transducers of said device, and sets defaults for the datepicker
   * @param  $scope controllers scope
   * @param  Device current device
   * @param  User   current user
   */
  app.controller('DeviceTimeSeriesCtrl',function($scope,User){
    $scope.user=User; 
    $scope.$watch('device',function(){
      if($scope.device != null){
        if(angular.isUndefined($scope.device.transducers)){
          $scope.device.getTransducers();
        }
      }
    });
  });
  app.controller('DeviceFunctionsCtrl',function($scope,$http,User,Alert,Command,$interval){
    $scope.user=User;
    $scope.$watch('device',function(){
      if($scope.device != null && angular.isUndefined($scope.device.transducers) ){
        $scope.device.getTransducers();
      }
    });

    // update transducers value each 3 minutes
    var i = $interval(function(){
      if($scope.device!=null && angular.isDefined($scope.device.transducers)){
        $scope.device.updateTransducers();
      }
    },UPDATE_TRANSDUCER_INTERVAL);

    // on scope change (destroy) cancel interval
    $scope.$on('$destroy', function(){ 
      $interval.cancel(i); 
    });

    $scope.objCommand=Command;
    /**
    * Valid if a value is set
    */
    $scope.isValid= function(command){
      if(typeof command!='undefined' && command.value!=''){
        return false;
      }
      return true;
    };
    /**
    * Run a command
    * get transducer data to pass to a command a then execute
    */
    $scope.runCommand=function(transducer,command){
      $scope.objCommand.transducer.device_id=$scope.device.id;
      $scope.objCommand.transducer.name=transducer.name;
      $scope.objCommand.transducer.value=transducer.lastValue;
      $scope.objCommand.transducer.min_value=transducer.min;
      $scope.objCommand.transducer.max_value=transducer.max;
      $scope.objCommand.transducer.unit=transducer.unit;
      if($scope.objCommand.transducer.unit == 'enum'){
        $scope.objCommand.transducer.values=transducer.enum;
      }

      if(typeof command!='undefined' && command.value!=''){
        $scope.objCommand.value=command.value;
        $scope.objCommand.run().success(function(response){
          if(response.error){
            Alert.close();
            Alert.open('warning','Command could not be executed.');
          }else{
            Alert.close();
            Alert.open('success','Command successfully executed.');
            if($scope.device != null){
              $scope.device.getTransducers();
            }
          }        
        });
      }      
    };    
  });
  app.controller('DevFavoritesModalCtrl',function($scope,$modalInstance,$http,$state,$stateParams,User,Alert,Device,Folder,Favorite,$q,$timeout){
    $scope.promise= Device.get($stateParams['id']);
    $scope.devBrowserFavorites = {};
    $scope.newFavorites = [];
    $scope.favorite = Favorite;
    $scope.favoritesToRemove = [];
    $scope.isAlreadySelected = false;
    $scope.errors = [];
    $scope.promise.then(function(object){
      $scope.device=object;
      $scope.device.folders= [];
    });  
    $scope.$watch(function(){ return Favorite.id;},function(){
      if($scope.favorite.id != null && typeof $scope.favorite.id !== 'undefined'){
        $scope.addFavorite($scope.favorite);
        $scope.favorite.id=null;
        $scope.favorite.name=null;

      }
    });
    $scope.$watch('device',function(){
      if($scope.device != null && angular.isUndefined($scope.device.folders) ){
        $scope.device.folders= [];
      }
    });
    
    $scope.myIndexOf = function(arr, obj){
      for(var i = 0; i < arr.length; i++){
        if(arr[i].id==obj.id){
          return i;
        }
      };
      return -1;
    }
    $scope.addFavorite = function($favorite){
      var favorite = {id:$favorite.id,name:$favorite.name};
      var indexNewFavorites = $scope.myIndexOf($scope.newFavorites,favorite);
      var indexDeletedFavorites = $scope.myIndexOf($scope.favoritesToRemove,favorite);
      var inFavorites = $scope.myIndexOf($scope.device.folders,favorite);
      if(indexDeletedFavorites!==-1){
        $scope.favoritesToRemove.splice(indexDeletedFavorites,1);
        return;
      }
      if(inFavorites!==-1 || indexNewFavorites!==-1){
        $scope.isAlreadySelected = true;
        return;
      }
      $scope.newFavorites.push(favorite);
      $scope.isAlreadySelected = false;
    }

    $scope.deleteFavorite=function($favorite){
      var indexNewFavorites = $scope.myIndexOf($scope.newFavorites,$favorite);
      if(indexNewFavorites!==-1){
        $scope.newFavorites.splice(indexNewFavorites,1);
      }
    };

    $scope.submit = function(){
      var requests = [];
      if($scope.newFavorites.length > 0){
        $scope.loading = true;
        for(favorite in $scope.newFavorites){
          requests.push({
            method: 'PUT',
            url: 'api/folder/' + $scope.newFavorites[favorite].id + '/add_device',
            folder: $scope.newFavorites[favorite].id,
            data: {
              device_id:$scope.device.id,
              two_way:false
            }
          });          
        }
      }
      doRequest(requests);
    };

    var doRequest = function(arrRequests){
      var defered = $q.defer();
      $timeout(function(){
        if(arrRequests.length<1){
          if($scope.errors.length<1){;
            Alert.close();
            Alert.open('success','Device successfully added to favorites');
          }else{
            if($scope.numRequests==1){
              Alert.open('warning','Device could not be added to favorite.');
            }else{
              var message = 'Device could not be added to some favorites: ';
              for(error in $scope.errors){
                message += $scope.errors[error];
                if(error!=$scope.errors.length-1){
                  message += ', ';
                }
              }
              Alert.close();
              Alert.open('warning',message);            
            }
          }
          defered.resolve(true);
          $modalInstance.dismiss();        
          return;   
        }else{
          current = arrRequests[0];
          $scope.setFavorite = $http({method:current.method, url:current.url, data: current.data ? current.data : {}})
           .success(function(response){
              if(response.error){
                $scope.errors.push(current.folder);
                console.log("Some errors occurred. " + response.message);
              }
              Folder.references[current.folder].getChildren();
              arrRequests.shift();
              doRequest(arrRequests);
           }).error(function(response){
              console.log('Some errors occurred. ' + response);
           });        
        }
      });
      return defered.promise;
    }
    $scope.cancel = function () {     
      $modalInstance.dismiss($scope.selectedFolder);
    };
  });
  /**
   * Controller to manage Device List
   * @param  service $scope       
   * @param  service $http        
   * @param  service $state       
   * @param  service $stateParams
   * @param  object  $window 
   * @param  service Alert
   * @param  factory User
   * @param  factory Folder
   */
  app.controller('DeviceListCtrl',function($scope,$http,$state,$stateParams,$window,Alert,User,Folder){
    $scope.user=User;
    $scope.devices=[];
    $scope.parentFolder=null;
    $scope.initFolder=function (intFolderId){
      $scope.loadFolder=$http.get('/api/device/'+intFolderId)
        .success(function(response){
          if(response['error']){
            Alert.open('warning',response.message);
            return;
          }
          response.device.children = [];
          response.device.type = 'location';
          $scope.devBrowser.currentNode =Folder.constructFolder(response.device);
          $scope.selectedFolder=$scope.devBrowser.currentNode;
        });
    };
    if(typeof $stateParams['folder'] !=='undefined' ){
      if(typeof Folder.references[$stateParams['folder']] !== 'undefined'){
        $scope.devBrowser.currentNode = Folder.references[$stateParams['folder']];
        $scope.selectedFolder=$scope.devBrowser.currentNode;
      }else{
        $scope.initFolder($stateParams['folder']);
      }
    } 
    $scope.$watch('devBrowser.currentNode',function(newObj,oldObjt){   
      // if(newObj !== oldObjt && $scope.devBrowser.currentNode.type=='location'){
      if(typeof $scope.devBrowser.currentNode != 'undefined' && $scope.devBrowser.currentNode.type=='location'){
        $scope.loadFolder=$http.get('/api/device/'+$scope.devBrowser.currentNode.id).success(
          function(response){
            if(response['error']){
              Alert.open('warning',response.message);
              return;
            }
            $scope.selectedFolder.id = response.device.id;
            $scope.folderDetail = response.device;
            if(typeof response.device.parent != 'undefined'){
              $scope.parentFolder=Folder.references[response.device.parent.id];
            }
          });
        $scope.isPublishOrOwner = typeof $scope.user.permittedDevices['owner'][$scope.selectedFolder.id] !== 'undefined' || typeof $scope.user.permittedDevices['publisher'][$scope.selectedFolder.id]  !== 'undefined'; 
        $scope.loadDevices=$scope.selectedFolder.getDevicesByFolder($scope.selectedFolder.id);
        $scope.loadDevices.success(function(object){
          $scope.devices=object.children;
        });        
      }
    });
    $scope.isOwner = function(deviceId){
      return typeof $scope.user.permittedDevices['owner'][deviceId] !== 'undefined';

    }
    $scope.isPublisher = function (deviceId){
      return typeof $scope.user.permittedDevices['publisher'][deviceId]  !== 'undefined';
    }
    $scope.removeFromFavorite = function(device){
      var confirm=$window.confirm('Are you sure you want to remove '+device.name+' ?');
      if(confirm){
        $scope.removeFavorite = $http.delete('/api/folder/'+$scope.selectedFolder.id+'/'+device.id)
        .success(function(response){
          if(response.error){
            Alert.open('warning',response.message);
            return;
          }
          $scope.selectedFolder.getChildren();
          $scope.devBrowser.selectNodeLabel($scope.selectedFolder);
          Alert.open('success',response.message);
        })
        .error(function(response){
          Alert.open('danger',response);
        });
      }
    };
    $scope.deleteFolder = function(){
      var confirm=$window.confirm('Are you sure you want to delete '+$scope.selectedFolder.name+' ?');
      if(confirm){
        if($scope.parentFolder != null){
          $scope.deleteReference = $http.delete('/api/folder/'+$scope.parentFolder.id+'/'+$scope.selectedFolder.id)
          .success(function(response){
            if(response.error){
              $window.alert(response.message);
              return ;
            }
          }).error(function(response){
            Alert.open('danger',response.message);
          });
        }
        $scope.deleteFolder=$http.delete('/api/folder/'+$scope.selectedFolder.id)
        .success(function(response){
          if(response.error){
            $window.alert(response.message);
            return;
          }
          delete $scope.selectedFolder;
          $scope.parentFolder.getChildren();
          $scope.devBrowser.selectNodeLabel($scope.parentFolder);
          Alert.open('success','Folder deleted');
        })
        .error(function(response){
            Alert.open('danger',response);
        });
      }      
    };
  });
  
  /**
   *Controller to manage device permissions
   *@param object $scope scope for this cntroller
   *@param service Device device service instance
   *@param object $stateParams parameters passed to state
   *@param object $modalInstance modal instance to manage
   *@param service $http service to perform http requests
   */
  app.controller('DeviceSetPermissionsCtrl', function($scope,Device,$stateParams,$modalInstance,User,$http){
    $scope.promise= Device.get($stateParams['id']);    
    $scope.promise.then(function(object){
      $scope.device=object;
    });  
    $scope.$watch('device',function(){
      if($scope.device != null && angular.isUndefined($scope.device.users) ){
        $scope.device.getUsers($stateParams['id']);
      }
    });     
    $scope.deviceId = $stateParams['id'];
    $scope.newUsersToAdd = [];
    $scope.usersToRemove = [];
    $scope.isAlreadySelected = false;
    $scope.allUsers = [];
    $scope.showUsers = false;

    $scope.loadUsers = function(){
      $scope.usersGet = $http.get('/api/user/').success(function(response){
        if(response.error){
          console.log("Some errors occurred. " + response.message);
        }else{
          for(user in response.users){
            var userItem = {};
            userItem.name = response.users[user].name;
            userItem.username = response.users[user].username;
            userItem.show = true;
            $scope.allUsers.push(userItem);
          }
          for(user in $scope.allUsers){
            for(permittedUser in $scope.device.users){
              if($scope.device.users[permittedUser].username == $scope.allUsers[user].username){
                angular.extend($scope.device.users[permittedUser],$scope.allUsers[user]);
              }
            }
          }
          $scope.showUsers = true;
        }
      }).error(function(response){
          console.log('Some errors occurred.' + response);
     });
    }; 

    $scope.addUser = function($item){
      var indexNewUsers = $scope.newUsersToAdd.indexOf($item);
      var hasPermission = $scope.isInArray($item.username,$scope.device.users);
      if(hasPermission){
        $scope.isAlreadySelected = true;
      }else if(indexNewUsers!==-1 && !$item.show){
        $item.show = true;
      }else if(indexNewUsers!==-1){
        $scope.isAlreadySelected = true; 
      }else if($scope.isInArray($item.username,$scope.usersToRemove)){
        indexToRemove = $scope.isInArray($item.username,$scope.usersToRemove);
        $scope.usersToRemove.splice($item);
        $scope.device.users.push($item);
      }else{
        $scope.newUsersToAdd.push($item);
        $scope.isAlreadySelected = false;      
      }
    }

    // Returns user position if exists, otherwise returns false
    $scope.isInArray = function(username,userArray){
      for(user in userArray){
        if(username==userArray[user].username){
          return user;
        }
      }
      return false;
    }

    $scope.removeUser = function(user){
      indexNewUsers = $scope.newUsersToAdd.indexOf(user);
      if(indexNewUsers!==-1){
        user.show = false;
      }else{
        indexPermittedUsers = $scope.device.users.indexOf(user);
        $scope.device.users.splice(indexPermittedUsers,1);
        $scope.usersToRemove.push(user);
      }
    }

    $scope.setPermissions = function(){
      var requests = [];
      if($scope.newUsersToAdd.length>0){
        for(user in $scope.newUsersToAdd){
          if($scope.newUsersToAdd[user].show){
            requests.push({
              method: 'POST',
              url: 'api/device/' + $scope.deviceId + '/add/' + $scope.newUsersToAdd[user].username
            });           
          }
        }
      }
      if($scope.usersToRemove.length>0){
        for(user in $scope.usersToRemove){
          requests.push({
            method: 'DELETE',
            url: 'api/device/' + $scope.deviceId + '/remove/' + $scope.usersToRemove[user].username
          });
        }          
      }
      doRequest(requests);
      $modalInstance.dismiss();      
    };

    var doRequest = function(arrRequests){
      if(arrRequests.length<1){
        if($scope.device != null){
         $scope.usersGet = $scope.device.getUsers($stateParams['id']);
        } 
        return;
      }else{
        for(item in arrRequests){
          current = arrRequests[item];
          $http({method:current.method, url:current.url, data: current.data ? current.data : {}})
           .success(function(response){
              if(response.error){
                console.log("Some errors occurred. " + response.message);
              }
              arrRequests.shift();
              doRequest(arrRequests);
           }).error(function(response){
              console.log('Some errors occurred. ' + response);
           });          
        }
      }
    }

    $scope.cancel = function(){
      $modalInstance.dismiss();
    }
  });

  app.controller('MulticontrolCtrl',function($scope,Folder,Device,Command,Alert,$interval){
    $scope.selectedDevices = {};
    $scope.devBrowserMulticontrol = {};
    $scope.labelSubmitBtn = 'Run commands';
    $scope.addDevice = function(device){
      $scope.promise = Device.get(device.id);
      $scope.promise.then(function(response){
        var device = {};
        angular.copy(response,device);
        var index = $scope.myIndexOf($scope.selectedDevices,device);
        if(index==-1){
          $scope.selectedDevices[device.id] = device;         
        }
      });
    }

    // update transducers value each 3 minutes
    var i = $interval(function(){
      var arrKeys = Object.keys($scope.selectedDevices);
      if(arrKeys.length>0){
        var devices = [];
        for(deviceKey in Object.keys($scope.selectedDevices)){
          if(angular.isDefined($scope.selectedDevices[arrKeys[deviceKey]].transducers)){
            devices.push($scope.selectedDevices[arrKeys[deviceKey]]);            
          }
        }
        $scope.updateTransducersValue(devices);        
      }
    },UPDATE_TRANSDUCER_INTERVAL);

    $scope.updateTransducersValue = function(devices){
      if(devices.length<1){
        return;
      }
      $scope.updateTransducersPromise = devices[0].updateTransducers();
      $scope.updateTransducersPromise.then(function(response){
        devices.shift();
        $scope.updateTransducersValue(devices);
      });
    }

    // on scope change (destroy) cancel interval
    $scope.$on('$destroy', function(){ 
      $interval.cancel(i); 
    }); 

    $scope.objCommand=Command;
    $scope.executing = null;
    $scope.runCommands = function(commands){
      if(commands.length<1){
        Alert.close();
        Alert.open('success','Commands execution finished.');
        $scope.objCommand.transducer.name=null;
        return;
      }
      var current = commands[0]; 
      $scope.objCommand.transducer.device_id = current.device;
      $scope.objCommand.transducer.name = current.transducer;
      $scope.objCommand.transducer.value = current.value;                
      $scope.objCommand.value = current.value;
      $scope.executing = $scope.objCommand.run();
      $scope.executing.then(function(response){
        console.log(response);
        var device = $scope.selectedDevices[$scope.objCommand.transducer.device_id];
        for(index in device.transducers){
          transducer = device.transducers[index];
          if(transducer.name==$scope.objCommand.transducer.name){
            if(response.data.error){
              transducer.status = 'error';
            }else{
              transducer.status = 'success';
            }
          }
        }
        commands.shift();
        $scope.runCommands(commands);      
      });    
    } 

    $scope.removeDevice = function(device){
      var index = $scope.myIndexOf($scope.selectedDevices,device);
      delete $scope.selectedDevices[device.id];
    }

    $scope.removeTransducer = function(device,transducer){
      transducer.id = transducer.name;
      var index = $scope.myIndexOf(device.transducers,transducer);
      device.transducers.splice(index,1);
      if(device.transducers.length<1){
        $scope.removeDevice(device);
      }
    }

    // This function (technically) does nothing, it's just a requirement 
    // for the device  browser to work properly
    $scope.selectedFolder = function(folder){}

    $scope.myIndexOf = function(arr, obj){
      for(var i = 0; i < arr.length; i++){
        if(arr[i].id==obj.id){
          return i;
        }
      };
      return -1;
    }
  });
})();