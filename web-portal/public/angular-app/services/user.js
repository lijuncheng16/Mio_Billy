/**
 * User Services
 */
(function(){
  var app = angular.module('user-services',['ui.router']);
  /**
   * Provides user info for controllers and app.
   * @return Obj User Model
   */
  app.factory('User',function($window,$http,Alert){
    var User = {};
    var sessionUser = JSON.parse($window.sessionStorage.getItem("User"));
    /*
     * Create user from session if it exists
     * only user data is actually stored, functions will be null,
     * so they must be redeclared.
     */
    if(sessionUser){
      User = sessionUser;
    }else{
      User.username = "";
      User.password = "";
      User.state = {};
      User.permittedDevices={publisher:{},owner:{},subscribe:{}};
      User.loggedIn = false;
    }
    /**
     * saves user data in sessionStorage
     * must be called when theres changes we want to persist.
     */
    User.saveSession = function(){
      $window.sessionStorage.setItem("User",JSON.stringify(User));
    };
    /**
     * Removes the user data from session
     */
    User.deleteSession = function(){
      User.username = "";
      User.password = "";
      User.permittedDevices={publisher:{},owner:{},subscribe:{}};
      User.loggedIn = false;
      $window.sessionStorage.removeItem("User");
    };
    /**
    * Check if the user is already subcribe to  a device
    */
    User.isSusbscribe=function(deviceId){
      if(typeof User.permittedDevices['owner'][deviceId] !== 'undefined'){
        return false;
      }
      if(typeof User.permittedDevices['subscribe'][deviceId] === 'undefined'){
        return true;
      } 
      return false;
    }
    /**
    * Check if the user i not owner and is unsubscribe
    */
    User.isUnSusbscribe=function(deviceId){
      if(typeof User.permittedDevices['owner'][deviceId] === 'undefined' && typeof User.permittedDevices['subscribe'][deviceId] !== 'undefined'){
        return true;
      }
      
      return false;
    }
    /**
     * Get the list of permitted devices to the user
     */
    User.getPermittedDevices = function(){
      return $http.get('/api/user/get_permitted_devices').success(function(response){
        if(response.error){
          Alert.open('warning','Could not get permitted devices, please refresh the page');
          return;
        }
        User.permittedDevices = response.devices;
        User.saveSession();
      });
    }
    /**
     * Request for event nodes
     * @todo implement this on device
     */
    User.getUserFavorites=function(){
     // $http.get('/api/user/'+User.username+'/get_available_nodes').success(function(response){
     //    if(response.error){
     //      console.log('there are some errors ' + response.message);
     //    }else{   
     //      for(favorite in response.folders){
     //        if(response.folders[favorite].name == 'Favorites'){
     //          User.favorites = response.folders[favorite];  
     //          User.saveSession();
     //        }            
     //      }          
     //    }        
     //  })
     //  .error(function(response){
     //    console.log('something wrong with the server.');
     //    console.log(response);
     //  });
    }; 

    // @todo use new endpoint on backend
    User.getUserNodes=function(){
     // $http.get('/api/user/'+User.username+'/get_available_nodes').success(function(response){
     //    if(response.error){
     //      console.log('Some errors occurred. ' + response.message);
     //    }else{
     //      var collections = [];
     //      for(var folder in response.folders){
     //        collections.push({
     //          name: response.folders[folder].name,
     //          id: response.folders[folder].id,
     //          folders: response.folders[folder].folders,
     //          type: response.folders[folder].type
     //        });
     //      }
     //      User.folderList = collections;
     //      User.saveSession();
     //    }       
     //  })
     //  .error(function(response){
     //    console.log('something wrong with the server');
     //    console.log(response);
     //  });
    }; 

    User.getMonitors = function(){
     var promise = $http.get('/api/monitor').success(function(response){
        if(response.error){
          console.log("Some errors occurred. " + response.message);
          return false;
        }else{ 
          User.monitors = {};
          for(monitor in response.monitors){
            User.monitors[response.monitors[monitor].id] = response.monitors[monitor];
          }
          User.saveSession();
        }
        return true;
      }).error(function(){
          console.log(response);
      });
      return promise;
    } 
    // run methods that need to be queried if session exists.
    if(sessionUser){
      User.getPermittedDevices();
      // User.getUserFavorites();
    }
    return User;
  });
})();