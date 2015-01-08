/**
 * Device Services
 */
(function(){
  var app=angular.module('device-services',[]);
  
  app.factory('Device',function($http,$q,Alert){
   
    function Device(json){
      angular.extend(this, json);  
    }
    Device.prototype={
      getFavorites:function(){
        var self = this;
        $http.get('/api/device/'+self.id+'/get_favorites')
        .success(function(response){
          if(response.error){
            console.log('Something wrong with the tool '+ response.message);
          }
          self.folders = [];
          for(var i in response.favorites){
            self.folders.push({id:response.favorites[i].id,name:response.favorites[i].name});
          }
        })
        .error(function(response){
          console.log('Something wrong with the server');
          console.log(response);
        });
      },
      getTransducers:function (){
        var self = this;
        $http.get('/api/device/'+self.id+'/get_transducers')
        .success(function(response){
          if(response.error){
            console.log('there are some errors '+response.message);
          }else{
            self.transducers=response.transducers;
          }
        })
        .error(function(response){
          console.log(response);
        });
      },      
      updateTransducers: function(){
        var self = this;
        return $http.get('api/device/'+ self.id + '/get_transducers_last_value').success(function(response){
          if(response.error){
            console.log('Some errors occured. ' + response.message);
          }else{
            for(t in self.transducers){
              for(tr in response.transducers){
                if(response.transducers[tr].id == self.transducers[t].name){
                  self.transducers[t].lastValue = response.transducers[tr].lastValue;
                }
              }
            }
          }
        }).error(function(response){
          console.log(response);
        });  
      },
      getUsers:function(device){
        var self = this;
        $http.get('/api/device/'+device+'/get_permitted_users')
        .success(function(response){
          if(response.error){
            console.log("Some errors occured. " + response.message);
          }else{
            self.users = [];
            for(userIndex in response.users){
              var user = {};
              user.name = response.users[userIndex].name;
              user.username = response.users[userIndex].username;
              self.users.push(user);
            }
          }
        })
        .error(function(response){
          console.log(response);
        });
      },
      subscribeUser:function(){
        var self =this;
        return $http.post('/api/device/'+self.id+'/subscribe')
        .success(function(response){
          return response;
        })
        .error(function(response){
          Alert.open('danger',response);
        });
      },
      unSubscribeUser:function(){
        var self =this;
        return $http.post('/api/device/'+self.id+'/unsubscribe')
        .success(function(response){
          return response;
        })
        .error(function(response){
          Alert.open('danger',response);
        });
      }
    };

    var DeviceService = {};
    DeviceService.get=function(deviceId){
      var defered = $q.defer();
      if(typeof deviceId == 'undefined' || deviceId == null){
        defered.resolve(false);
        return defered.promise;
      }
      
      return $http.get('/api/device/'+deviceId).then(function(response){
        if(response.data.error){
          console.log(response.data.message);
          return;
        }
        var objDevice = response.data.device;
        
        return new Device(objDevice);
      });
     
    };
    return DeviceService;
  });
})();