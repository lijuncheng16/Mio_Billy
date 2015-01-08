/**
* Schedule Task Controller
*/
(function(){
	var app = angular.module('schedule-task',['ui.router','mortar-services','cgBusy','angularTreeview','ui.bootstrap','mortar-directives']);	
	app.controller('ScheduleTaskCtrl',function($scope,$state,$stateParams,Schedule,Folder,Device,Alert){
		$scope.isMulti=true;
		$scope.step= 1;
		$scope.scheduleTask = {};
		$scope.schedule = {freq:'DAILY',interval:1};
	 	$scope.calendar = {};
  	$scope.calendar.dayOfWeek ={SU:true,MO:true,TU:true,WE:true,TH:true,FR:true,SA:true};
  	$scope.calendar.months = {'1':true,'2':true,'3':true,'4':true,'5':true,'6':true,'7':true,'8':true,'9':true,'10':true,'11':true,'12':true};
		$scope.validator = {};
		$scope.validator.ends='never';
		$scope.selectedDevices = {};
		
		$scope.promise = null;
		$scope.labelSubmitBtn='Create schedule';
		$scope.addDevice = function(device){
      $scope.promise = Device.get(device.id);
      $scope.promise.then(function(response){
        var device = {};
        angular.copy(response,device);
        if(typeof $scope.selectedDevices[device.id] === 'undefined'){
          $scope.selectedDevices[device.id] = device;         
        }
      });
    };
		$scope.removeDevice = function(device){
    	if(typeof $scope.selectedDevices[device.id] != 'undefined'){        
    		delete $scope.selectedDevices[device.id];
    	}
    };
    $scope.removeTransducer = function(device,transducer){
      transducer.id = transducer.name;
      var index = $scope.myIndexOf(device.transducers,transducer);
      device.transducers.splice(index,1);
      if(device.transducers.length<1){
        $scope.removeDevice(device);
      }
    };
    $scope.isScheduleRequired = function(){
    	return typeof $scope.schedule.time == 'undefined' || typeof $scope.schedule.info == 'undefined';
    };
    $scope.createSchedule = function(commands){
    	if(commands.length<1){
        Alert.close();
        Alert.open('success','Execution finished, check each transducer to know the status');
        return;
    	}
    	var current = commands[0];
    	$scope.schedule.t_value = current.value;
    	$scope.schedule.t_name = current.transducer;
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
    	$scope.executing = Schedule.multiSubmit($scope.schedule,current.device);
    	$scope.executing.then(function(response){
        console.log(response);
        var device = $scope.selectedDevices[current.device];
        for(index in device.transducers){
        	transducer = device.transducers[index];
        	if(transducer.name==$scope.schedule.t_name){
          	if(response.data.error){
            		transducer.status = 'error';
          	}else{
            		transducer.status = 'success';
          	}
        	}
        }
        commands.shift();
        $scope.createSchedule(commands);      
      }); 
    };
    $scope.myIndexOf = function(arr, obj){
      for(var i = 0; i < arr.length; i++){
        if(arr[i].id==obj.id){
          return i;
        }
      };
      return -1;
    };
    // This function in this content is not use, its just a requirement 
  	// for the device  browser to work properly
  	$scope.selectedFolder = function(folder){};
		$scope.goToSelectDevices = function(){
			$scope.step=2;
		};
		$scope.comeBack=function(){
			$scope.step=1;
		};
	});

})();