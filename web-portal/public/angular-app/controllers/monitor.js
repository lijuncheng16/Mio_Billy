/**
* Monitor Controller module
*/
(function(){
  var app = angular.module('monitor-controller', ['ui.router','mortar-services','cgBusy','angularTreeview','ui.bootstrap','mortar-directives']);
  /*
   * Controller to list monitors of an user
   * @param object $scope scope for the controller
   * @param service User service instance to  get monitors from
   * @param service Monitor service instance to manage monitors actions
   * @param service Folder service instance to manage user folders
  */
  app.controller('MonitorListCtrl', function($scope,User,Monitor,Folder){
    $scope.user = User;
    $scope.devBrowserMonitor = {};
    $scope.monitors = $scope.user.monitors;
    $scope.$watch('user.monitors',function(newObject,oldObject){
      $scope.monitors = $scope.user.monitors;
    });
    // This function is implemented on child controllers. 
    $scope.addDevice = function(device){}
    // This function (technically) does nothing, its just a requirement 
    // for the device  browser to work properly
    $scope.selectedFolder = function(folder){}
  });

  /*
   * Controller to view a monitor's detailed data
   * @param object $scope scope for the controller
   * @param object $stateParams object to get params passed to this state
   * @param service Monitor service instance to manage monitors actions
   * @param object $state service instance to manage state actions
   * @param service $modal modal instance to manage modal windows
   * @param service $window service instance to manage window's actions and utilities
   * @param service Alert service instance to alert messages
   * @param service User service instance to  get monitors from
   * @param service Device  service instance to manage device actions
  */
  app.controller('MonitorViewCtrl', function($scope,$stateParams,Monitor,$state,$modal,$window,Alert,User,Device){
    $scope.monitor = {};
    $scope.devices = {};
    $scope.promise = Monitor.get($stateParams['id'],true);
    $scope.unsavedTransducers = false;
    $scope.promise.then(function(promise){
      if(typeof promise.data != 'undefined'){
        $scope.monitor = promise.data.monitor;
      }else{
        $scope.monitor = promise;
      }
      if(typeof $scope.monitor!='undefined'){
        for(transducer in $scope.monitor.transducers){
          if(typeof $scope.devices[$scope.monitor.transducers[transducer].device]=='undefined'){
            $scope.devices[$scope.monitor.transducers[transducer].device] = {
              id: $scope.monitor.transducers[transducer].device,
              transducers: []
            } 
          }
          // Request device data to set name property on devices list
          $scope.devicePromise = Device.get($scope.monitor.transducers[transducer].device);
          $scope.devicePromise.then(function(objDevice){
            $scope.devices[objDevice.id].name = objDevice.name;
          });
          $scope.devices[$scope.monitor.transducers[transducer].device].transducers.push($scope.monitor.transducers[transducer]);
        }        
      }
    }); 
    $scope.$parent.addDevice = function(device){
      $modal.open({
        templateUrl:'angular-app/partials/transducers-select.html',
        controller:'MonitorAddTransducersCtrl',
        resolve: {
          deviceId: function(){
            return device.id;
          },
          monitorId: function(){
            return $stateParams.id;
          }
        }
      }).result.then(function(result){
        if(result){
          $scope.unsavedTransducers = result;
        }
      });
    }

    // Open up a modal to edit a monitor
    $scope.edit = function(){
      $modal.open({
        templateUrl:'angular-app/partials/monitor-edit.html',
        controller:'MonitorEditCtrl',
        resolve: {
          monitorId: function(){
            return $stateParams.id;
          }
        }
      }).result.then(function(result){
          console.log(result);
          $state.go($state.current, {}, {reload: true});
        },function(){});
    }

    // Deletes a selected monitor
    $scope.delete = function(){
      if($window.confirm('Are you sure to delete '+$scope.monitor.name+' from monitors?')){
        $scope.delete = Monitor.delete($scope.monitor.id);
        $scope.delete.then(function(response){
          if(response.data.error){
            Alert.open('warning','Fail to delete monitor.');
          }else{
            $scope.getUsers = User.getMonitors();
            $scope.getUsers.then(function(response){
              $state.transitionTo('monitor.view');
              Alert.open('success','Monitor successfully deleted.');            
            });
          }          
        });        
      }
    } 

    // Save new transducers to monitor
    $scope.saveChanges = function(){
      $scope.monitor.addNewTransducers = true;
      $scope.saveMonitor = $scope.monitor.save();
      $scope.saveMonitor.then(function(response){
        if(response.data.error){
          Alert.open('warning',response.data.message);
        }else{
          $scope.unsavedTransducers = false;
          Alert.open('success',response.data.message);
          User.getMonitors();
          $state.go($state.current, {}, {reload: true});
        }
      });
    }
  });

  app.controller('MonitorEditCtrl',function($scope,$modalInstance,monitorId,Monitor,Alert,User,Device){
    $scope.transducersToRemove = [];
    $scope.monitorName = '';
    $scope.promise = Monitor.get(monitorId,true);
    $scope.promise.then(function(promise){
      if(typeof promise.data != 'undefined'){
        $scope.monitor = promise.data.monitor;
      }else{
        $scope.monitor = promise;
      }
      for(transducer in $scope.monitor.transducers){
        if(typeof $scope.monitor.transducers[transducer].deviceName=='undefined'){
          $scope.devicePromise = Device.get($scope.monitor.transducers[transducer].device);
          $scope.devicePromise.then(function(objDevice){
            for(t in $scope.monitor.transducers){
              if($scope.monitor.transducers[t].device==objDevice.id){
                $scope.monitor.transducers[t].deviceName = objDevice.name;
              }
            }
          });          
        }
      }
      $scope.monitorName = $scope.monitor.name;
    });

    $scope.saveChanges = function(name){
      if(name!=$scope.monitorName && name!=''){
        $scope.monitor.newName = name;
      }
      if($scope.transducersToRemove.length>0){
        $scope.monitor.removeTransducers = true;
        for(trans in $scope.transducersToRemove){
          var index = myIndexOf($scope.monitor.transducers,$scope.transducersToRemove[trans]);
          if(index != -1){
            $scope.monitor.transducers[index].remove = true;
          }
        }
      }
      $scope.result = $scope.monitor.save();
      $scope.result.then(function(response){
        $modalInstance.close(true);
        if(response.data.error){
          Alert.open('warning','Fail to update monitor.');
        }else{          
          User.getMonitors();
          Alert.open('success','Monitor successfully updated.');
        }
      });
    }

    $scope.cancel = function(){
      $modalInstance.dismiss();
    } 
  });

  app.controller('MonitorAddTransducersCtrl',function($scope,Monitor,$modalInstance,Device,deviceId,monitorId){
    $scope.newTransducers = [];
    $scope.device=null;    
    $scope.promiseDevice = Device.get(deviceId);
    $scope.promiseDevice.then(function(object){
      $scope.device=object;
      $scope.device.getTransducers();
    });
    $scope.monitor = {};
    $scope.promiseMonitor = Monitor.get(monitorId);
    $scope.promiseMonitor.then(function(promise){
      if(typeof promise.data!='undefined'){
        $scope.monitor = promise.data.monitor;
      }else{
        $scope.monitor = promise;
      }
    });

    // Adds new transducers to monitor
    $scope.addTransducers = function(){ 
      if($scope.newTransducers.length>0){
        var tempArray = [];
        for(transducer in $scope.newTransducers){
          $scope.newTransducers[transducer].device = deviceId;
          $scope.newTransducers[transducer].id = $scope.newTransducers[transducer].name;
          var index = myIndexOf($scope.monitor.transducers,$scope.newTransducers[transducer]);
          if(index==-1){
            tempArray.push($scope.newTransducers[transducer]);                              
          }
        }
        if(tempArray.length>0){
          $scope.monitor.addTransducers(tempArray);             
        } 
      }
      $modalInstance.close($scope.monitor.unsavedTransducers);
    }
   
    $scope.cancel = function(){
      $modalInstance.dismiss();
    }    
  });

  // Controller to create a brand new monitor
  app.controller('MonitorAddCtrl',function($scope,Monitor,$state,$http,$modal,Alert,User,Device){
    $scope.monitorName = '';
    $scope.transducers = [];
    $scope.devices = {};

    $scope.$parent.addDevice = function(device){
      $modal.open({
        templateUrl:'angular-app/partials/transducers-select.html',
        controller:'NewMonitorAddTransducersCtrl',
        resolve: {
          deviceId: function(){
            return device.id;
          }
        }
      }).result.then(function(result){
        var newTransducers = result;
        if(newTransducers.length>0){
          for(transducer in newTransducers){
            newTransducers[transducer].id = newTransducers[transducer].name;
            var index = myIndexOf($scope.transducers,newTransducers[transducer]);
            if(index==-1){
              $scope.transducers.push(newTransducers[transducer]);
              for(transducer in $scope.transducers){
                if(typeof $scope.devices[$scope.transducers[transducer].device]=='undefined'){
                  $scope.devices[$scope.transducers[transducer].device] = {
                    id: $scope.transducers[transducer].device,
                    transducers: []
                  } 
                }
                // Request device data to set name property on devices list
                $scope.devicePromise = Device.get($scope.transducers[transducer].device);
                $scope.devicePromise.then(function(objDevice){
                  $scope.devices[objDevice.id].name = objDevice.name;
                });
                $scope.devices[$scope.transducers[transducer].device].transducers.push($scope.transducers[transducer]);
              }
            }              
          }
        }
      });
    }    

    $scope.saveMonitor = function(){
      if($scope.monitorName!=''){
        var data = {};
        data.transducers = [];
        data.name = $scope.monitorName;
        if($scope.transducers.length>0){
          for(transducer in $scope.transducers){
            data.transducers.push({
              id: $scope.transducers[transducer].name,
              name: $scope.transducers[transducer].name,
              device: $scope.transducers[transducer].device
            });
          }          
        }

        $scope.create = Monitor.create(data);
        $scope.create.then(function(response){
          if(response.data.error){
            Alert.open('warning','Fail to create monitor');
          }else{
            Alert.open('success','Monitor '+ $scope.monitorName +' successfully created.');
            $scope.promise = User.getMonitors();
            $scope.promise.then(function(data){
              Monitor.updateList();
              $state.transitionTo('monitor.view',{id: response.data.monitor.id});              
            });
          }
        });
      }      
    }  
  });
  
  // Controller to add transducers to new monitor
  app.controller('NewMonitorAddTransducersCtrl', function($scope,$modalInstance,deviceId,Device){
    $scope.device = {};
    $scope.promise = Device.get(deviceId);
    $scope.promise.then(function(promise){
      $scope.device = promise;
      $scope.device.getTransducers();
    });
    $scope.newTransducers = [];

    $scope.addTransducers = function(){
      if($scope.newTransducers.length>0){
        for(transducer in $scope.newTransducers){
          $scope.newTransducers[transducer].device = deviceId; 
        }
      }
      $modalInstance.close($scope.newTransducers);
    }

    $scope.cancel = function(){
      $modalInstance.dismiss();
    } 
  });

  // function to evaluate is an element with certain id exists in array
  var myIndexOf = function(arr, obj){
    for(var i in arr){
      if(arr[i].id==obj.id && arr[i].device == obj.device){
        return i;
      }
    };
    return -1;
  }
})();