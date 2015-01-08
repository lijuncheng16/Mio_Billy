(function(){
  var app = angular.module('schedule-services',['alert-handler']);
  app.factory('Schedule',function($http,Alert,$window){
    var schedules = {};
    var device = '';
    
    var Schedule = function(json){
      angular.extend(this,json);
      this.time = new Date(this.time);
      if(typeof this.byday !== 'undefined' && this.byday !== null){
        this.byday = this.byday.split(',');
      }
      if(typeof this.bymonth !== 'undefined' && this.bymonth !== null){
        this.bymonth = this.bymonth.split(',');
      }
      if(typeof this.exdate !== 'undefined' && this.exdate !== null){
        this.exdate = this.exdate.split(',');
      }
      if(typeof this.until !== 'undefined' && this.until !== null){
        this.until = new Date(this.until);
      }
    }
    
    var getSchedules = function(deviceId){ 
      device = deviceId;
      $http.get('/api/schedule/'+device)
        .success(function(response){
          if(response.error){
            Alert.open('warning',response.message);
            return;
          }
          angular.copy({},schedules);
          for(var s in response.schedules){ 
            schedules[response.schedules[s].id] = new Schedule(response.schedules[s]);
          }           
        });
    }
    var submitMulti = function(schedule,deviceId){
      return $http.post('api/schedule/'+deviceId,schedule);
    }
    var saveSchedule = function(schedule){
      var isUpdate = true;
      if(typeof schedule.id === 'undefined'){
        isUpdate=false;
      }
      if(isUpdate){
        $http.put('api/schedule/'+device+'/'+schedule.id,schedule)
        .success(function(response){
          result =response.error;
          if(response.error){
            Alert.open('warning','The schedule could not be edited :' + response.message);
            return false;
          }
          Alert.open('success',response.message);
          
        })
        .error(function(response){
            console.log('Error with the server: '+response);
        });

      }else{
        $http.post('api/schedule/'+device,schedule)
        .success(function(response){
          result =response.error;
          if(response.error){
            Alert.open('warning','The schedule could not be create :' + response.message);
            return false;
          }
          getSchedules(device);
          Alert.open('success',response.message);
          
        })
        .error(function(response){
            console.log('Error with the server: '+response);
        });
      }
      return true;
    };
    var removeSchedule = function(id){
      var confirm = $window.confirm('Are you sure you want to delete this Schedule?');
      if(confirm){
        $http.delete('api/schedule/'+device+'/'+id)
        .success(function(response){
          if(response.error){
            Alert.open('warning',response.message);
            return;
          }
          getSchedules(device);
          Alert.open('success','Schedule deleted');
        });
      }
    };
    var getSchedule = function(id){
      return schedules[id];
    }
    return {
      getSchedules : getSchedules,
      schedules: schedules,
      get:getSchedule,
      save:saveSchedule,
      remove:removeSchedule,
      multiSubmit:submitMulti
    }
  });
  // app.service('Schedule',function($http,$window,Alert){
  //   var device=null;
  //   var uid=0;
  //   this.dtstart='';
  //   this.dtstamp='';
  //   this.summary='';
  //   this.description='';
  //   this.rrule = [];
  //   this.exdate = [];
  //   var schedules={};
  //   /**
  //    * Default construct object to set all default values
  //    */
  //   this.Schedule=function(){
  //     uid=0;
  //     this.dtstart='';
  //     this.dtstamp='';
  //     this.summary='';
  //     this.description='';
  //     this.rrule = [];
  //     this.exdate = [];
  //   };
  //   /**
  //    * return an instance of schedule
  //    * @param  int intUid unique identifier of schedule object
  //    * @return object the match schedule or new schedule
  //    */
  //   this.get=function(intUid){
  //     if(typeof intUid == 'undefined'|| intUid ==null || typeof schedules[intUid] == 'undefined' || schedules[intUid]==null ){
  //       this.Schedule();
  //       return this;
  //     }
  //     return schedules[intUid];
  //   };
  //   /**
  //    * Set Unique identifier of the Schedule
  //    * @param int uid unique identifier
  //    */
  //   this.setUid=function(intUid){
  //     uid = intUid;
  //   };
  //   /**
  //    * Get current unique identifier
  //    * @return int unique identifier of a schedule
  //    */
  //   this.getUid=function(){
  //     return uid;
  //   };
  //   /**
  //    * Get the list of schedule available
  //    * @return array list of schedule
  //    */
  //   this.getSchedules=function(){
  //     return schedules;
  //   };
  //   /**
  //    * Set the device of the schedule
  //    * @param object objDevice Device service
  //    */
  //   this.setDevice=function(objDevice){
  //     device = objDevice;
  //   };
  //   /**
  //    * Get device
  //    * @return service Device instance
  //    */
  //   this.getDevice=function(){
  //     return device;
  //   };
  //   /**
  //    * Add a new exception date
  //    * @param date newException new date or array of date
  //    */
  //   this.addException=function(newException){
  //     this.exdate.push(newException);
  //   };
  //   /**
  //    * Remove a exception from exdate array
  //    * @param  int index index in array
  //    */
  //   this.removeException=function(index){
  //     this.exdate.splice(index,1);
  //   };
  //   /**
  //    * Load all the schedule from a device
  //    */
  //   this.loadSchedule=function(objDevice){
  //     var schedule=this;
  //     $http.get('/api/schedule/'+objDevice.name)
  //       .success(function(response){
  //         if(response.error){
  //           console.log('There are problem in the server '+response.message);
  //         }else{
  //           for(var s in response.schedules){ 
  //             var date=new Date(response.schedules[s].dtstart);  
  //             var month = date.getMonth(); //month in js are from 0 to 11 
  //             month +=1;
  //             uid= response.schedules[s].uid;
  //             schedule.dtstart=date.getFullYear()+''+month+''+date.getDate();
  //             schedule.dtstamp=response.schedules[s].dtstamp;
  //             schedule.summary=response.schedules[s].summary;
  //             schedule.description=response.schedules[s].description;
  //             schedule.rrule =response.schedules[s].rrule;
  //             schedule.exdate = response.schedules[s].exdate; 
  //             device=objDevice;    
  //             schedules[uid]=schedule;
  //           } 
  //         }
  //       }
  //     ).
  //     error(function(response){
  //       console.log(response);
  //     });
  //   };
  //   //CRUD
  //   /**
  //    * Create a schedule
  //    */
  //   this.create=function(){
  //     var result=false;
  //     $http.post('api/schedule/'+device.name,{
  //       schedule:{
  //       dtstart:this.dtstart,
  //       summary:this.summary,
  //       description:this.description,
  //       rrule:this.rrule,
  //       exdate:this.exdate
  //     }})
  //     .success(function(response){
  //       result =response.error;
  //       if(response.error){
  //         $window.alert('The schedule could not be create :' + response.message);
  //       }else{
  //         $window.alert(response.message);
  //       }
  //     })
  //     .error(function(response){
  //         console.log('Error with the server: '+response);
  //     });
  //     return result;
  //   };
  //   /**
  //    * Edit schedule
  //    */
  //   this.edit=function(){
  //     var result=false;
  //     $http.put('api/schedule/'+device.name+'/'+uid,{
  //       schedule:{
  //       dtstart:this.dtstart,
  //       summary:this.summary,
  //       description:this.description,
  //       rrule:this.rrule,
  //       exdate:this.exdate
  //     }})
  //     .success(function(response){
  //       result=response.error;
  //       if(response.error){
  //         $window.alert('The schedule could not be edit :' + response.message);
  //       }else{
  //         $window.alert(response.message);
  //       }
  //     })
  //     .error(function(response){
  //       console.log('Error with the server: '+response);
  //     });
  //     return result;
  //   };
  //   /**
  //    * Remove Schedule
  //    */
  //   this.remove=function(){
  //     $http.delete('api/schedule/'+device.name+'/'+uid)
  //     .success(function(response){
  //       if(response.error){
  //         $window.alert('The schedule could not be remove :' + response.message);
  //       }else{
  //         delete schedules[uid];
  //         $window.alert(response.message);
  //       }
  //     })
  //     .error(function(response){
  //       console.log('Error with the server: '+response); 
  //     });
  //   };
  // });
})();