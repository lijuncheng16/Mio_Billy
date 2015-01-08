/**
* Monitor services
*/
(function(){
  var app = angular.module('monitor-services',[]);

  app.factory('Monitor', function($http,User,Device,$q){
    var listMonitors = {};

    var Monitor = function(json){
      angular.extend(this,json);
    };
    Monitor.prototype = {
      transducers: [],
      unsavedTransducers: false,
      addTransducers: function(transducers){
        for(transducer in transducers){
          transducers[transducer].isNew = true;
          this.transducers.push(transducers[transducer]);
        }
        this.unsavedTransducers = true;
      },
      save: function(){
        if(typeof this.id=='undefined'){
          var params = {};
          params.name = this.name;
          params.transducers = this.transducers;
          var promise = $http.post('api/monitor',params).success(function(response){
            if(response.error){
              console.log('Some errors occurred. ' + response.message);
            }else{
              listMonitors[response.monitor.id] =  new Monitor(response.monitor);
            }
            return response;
            }).error(function(response){
                console.log(response);
            });
          return promise;
        }else{
          var params = {};
          if(typeof this.newName!='undefined'){
            params.name = this.newName;
          }
          if(this.removeTransducers){
            remove_transducers = {};
            for(transducer in this.transducers){
              if(this.transducers[transducer].remove){
                if(typeof remove_transducers[this.transducers[transducer].device] == 'undefined'){
                  remove_transducers[this.transducers[transducer].device] = [];
                }
                remove_transducers[this.transducers[transducer].device].push(this.transducers[transducer].name);
              }
            }
            params.remove_transducers = remove_transducers;
            this.removeTransducers = false;
          }
          if(this.addNewTransducers){
            transducers = [];
            for(transducer in this.transducers){
              if(this.transducers[transducer].isNew){
                transducers.push({
                  id: this.transducers[transducer].name,
                  name: this.transducers[transducer].name,
                  device: this.transducers[transducer].device
                });            
              }
            }
            params.transducers = transducers; 
            this.addNewTransducers = false;
            this.unsavedTransducers = true;         
          }
          var promise = $http.put('/api/monitor/'+this.id,params).success(function(response){
            if(response.error){
              console.log("Some errors occurred. " + response.message);
              return;
            }
            return response;
            }).error(function(response){
              console.log(response);
            });
          return promise;
          this.unsavedTransducers = false;
          return;          
        }
      }
    }
    var MonitorFactory = {
      updateList: function(){
        listUserMonitors = User.monitors;
      },
      get:function(id){
        var deferred = $q.defer();
        if(typeof User.monitors[id]=='undefined'){
          deferred.resolve(false);
          return deferred.promise;
        }
        if(typeof listMonitors[id] != 'undefined'){           
          deferred.resolve(listMonitors[id]);
          return deferred.promise;
        }
        $http.get('/api/monitor/'+id).success(function(response){
          if(response.error){
            console.log("Some errors occurred. " + response.message);
            return;
          }
          listMonitors[id] = new Monitor(response.monitor);
          deferred.resolve(listMonitors[id]);
        }).error(function(response){
            console.log(response);         
        });
        return deferred.promise;
      },
      delete: function(id){
        var promise = $http.delete('/api/monitor/'+id).success(function(response){
          if(response.error){
            console.log("Some errors occurred. " + response.message);
          }else{
            console.log(response.message);
          }
          return response;
        }).error(function(response){
          console.log(response);
        });
        return promise;
      },
      create: function(data){
        var monitor =  new Monitor(data);
        return monitor.save();
      }
    };
    return MonitorFactory;
  });
})();