/**
 * User controller module
 */
(function(){
  var app = angular.module('user-controller',['ui.router','mortar-services','mortar-directives','cgBusy','angularTreeview','ui.bootstrap']);
  /**
   * Controller for managing user list
   * @param  object $scope    scope of the controller
   * @param  factory User     User factory instance
   * @param  service $http    angulars $http service
   */
    app.controller('UserListCtrl', function($scope,$http,$modal,$stateParams,Alert){
      
      $scope.getUsers = function(){
        $scope.usersGet = $http.get('/api/user/').success(function(response){
          if(response.error){
            Alert.open('warning','User list could not be loaded. Try reloading the page');
            console.log("Some errors occurred. " + response.message);
          }else{            
            $scope.userList = response.users; 
          }
        }).error(function(response){
            Alert.open('warning','User list could not be loaded. Try reloading the page');
            console.log(response);
       });
      };

      $scope.removeUser = function(username){
        if(confirm('Are you sure to delete this user?')){
          $scope.userDelete = $http.delete('api/user/' + username).success(function(response){
            if(response.error){
              console.log("Some errors occurred. " + response.message);
            }else{
              $scope.getUsers(); // reload user list after deletion
            }
          }).error(function(response){
              console.log('Some errors occurred.' + response);
          });
        }
      };
    });
    /**
     * Controller to search user's devices
     * @param  object $scope    scope of the controller
     * @param  service $http    angulars $http service 
     * @param  object  $state   state object
     * @param  object $stateParams parameters of the current state 
     */
    app.controller('UserDevicesCtrl',function($scope,$http,$state,User){
      $scope.user=User;
      $scope.viewDevice = function($item){
        $scope.searchDevice = '';
        $state.go('device.view.detail',{id:$item.id});
      }
    });
    /**
     * Controller to show user detail
     * @param  object $scope    scope of the controller
     * @param  service $http    angulars $http service 
     * @param  object $stateParams parameters of the current state 
     */
    app.controller('ViewUserCtrl', function($scope,$http,$stateParams,User,$state){
      $scope.user = {};
      $scope.showDevices = false;
      if($stateParams.username!=User.username){
        $scope.getUserDetail = function(){
          $scope.userDetailGet = $http.get('/api/user/' + $stateParams.username).success(function(response){
            if(response.error){
              console.log("Some errors occurred. " + response.message);
            }else{
              $scope.user = response.user;
            }
          }).error(function(response){        
              console.log('Some errors occurred.' + response);
          });
        };
        $scope.getUserDetail();
      }else{
        $scope.showDevices = true;
        $scope.user = User;
      }

      $scope.viewNodeDetail = function(node){
        if(node.type=='location'){
          $state.go('device.list',{folder:node.id});
        }else{
          $state.go('device.view.detail',{id:node.id});
        }
      }
    });    

    /**
     * Controller to create and update user
     * @param  object  $scope       scope of the controller
     * @param  object  $modalInstance modal instance to manage 
     * @param  service $http        angular $http service 
     * @param  string  username user username to load on edit 
     */
    app.controller('AddEditUserCtrl', function($scope,$modalInstance,$http,$state,username,User){
      $scope.userIsAdmin = (User.group=='admin') ? true : false;
      $scope.user = {};
      $scope.edit = false;
      $scope.passwordEdit = false;
      $scope.modalLabel = '';
      $scope.userTypes = [
        {value: 'admin',label:'Administrator'},
        {value: 'user',label:'User'},
        {value: 'installer',label:'Installer'}
      ];
      // Set boolean variable passwordEdit to true or false
      $scope.editPassword = function(){
        $scope.passwordEdit = !$scope.passwordEdit;
      };

      // If username passed is defined, load this user's data
      if(username){
        $scope.getUser = function(username){
          $scope.userDetailGet = $http.get('/api/user/' + username).success(function(response){
            if(response.error){
              console.log("Some errors occurred. " + response.message);
            }else{
              $scope.user = response.user;              
            }
          }).error(function(response){        
              console.log('Some errors occurred.' + response);
          });
        };
        $scope.user = $scope.getUser(username);
        $scope.modalLabel = 'Edit';
        $scope.edit = true;
      }else{
        $scope.modalLabel = 'Add';
      }

      // Save user data, this handles both user creation and editing
      $scope.saveUser = function(){
        var postParams = {
            name: $scope.user.name,
            email: $scope.user.email,
            group: $scope.user.group
          };
        if($scope.edit){
          if($scope.passwordEdit){
            postParams.password = ($scope.user.password) ? $scope.user.password : '';
            postParams.password2 = ($scope.user.password2) ? $scope.user.password2 : '';
          }

          $scope.userEdit = $http.put('api/user/' + $scope.user.username, postParams).success(function(response){
            if(response.error){
              console.log('Some errors occurred. ' + response.message);
            }else{
              console.log(response.message);
              $modalInstance.close(true);
            }
          }).error(function(response){
            console.log(response);
          });  

        }else{
          postParams.username = $scope.user.username;
          postParams.password = $scope.user.password;
          postParams.password2 = $scope.user.password2;

          $scope.userPost = $http.post('api/user/create_user', postParams).success(function(response){
            if(response.error){
              console.log('Some errors occurred. ' + response.message);
            }else{
              console.log(response.message);
              $modalInstance.close(true);
            }
          }).error(function(response){
            console.log(response);
          }); 
        }
      };

      // Close modal form on cancel
      $scope.cancel = function(){
        $modalInstance.close(true);
      };
    });
    
  // controller to add device permission to user
  app.controller('PermitDevicesCtrl', function($scope,Folder,$modalInstance,$http,username){
    $scope.username = username;
    $scope.folders = Folder.folders;
    $scope.references=Folder.references;
    $scope.permittedDevices = [];
    $scope.devBrowser = {};
    $scope.alreadySelected = false;
    $scope.devicesToRemove = [];
    $scope.newDevicesToAdd = [];
    $scope.getAllFolders = function(){
      Folder.getChildren();
    };

    $scope.getPermittedDevices = function(){
      $scope.devicesGet = $http.get('api/user/' + $scope.username + '/get_permitted_devices').
        success(function(response){
          if(response.error){
            console.log('Some errors occurred. '+response.message);
          }else{
            for(device in response.object){
              response.object[device].show = true;
            }
            $scope.permittedDevices = response.object;  
          }
        }).error(function(response){
          console.log(response);
        });
    };

    $scope.$watch('devBrowser.currentNode', function( newObj, oldObj) {
      if( $scope.devBrowser && angular.isObject($scope.devBrowser.currentNode) ) {
        $scope.addDevice($scope.devBrowser.currentNode);
      }
    });

    $scope.addDevice = function(node){
      if(node.type=='device'){
        index = $scope.newDevicesToAdd.indexOf(node);
        if(index === -1){
          if($scope.devicesToRemove.indexOf(node)!==-1){
            $scope.devicesToRemove.splice($scope.devicesToRemove.indexOf(node),1);
            $scope.permittedDevices.push(node);
          }else{
            node.show = true;
            $scope.newDevicesToAdd.push(node);
            $scope.alreadySelected = false;             
          }
        }else if(index !== -1 && node.show==false){ 
          $scope.newDevicesToAdd[index].show = true;
        }else{
          $scope.alreadySelected = true;
        }       
      }else{
        for(child in node.folders){
          if(node.folders[child].type=='device'){
            index = $scope.newDevicesToAdd.indexOf(node.folders[child]);
            if(index === -1){ // If node doesn't exist in selection array
              node.folders[child].show = true;
              $scope.newDevicesToAdd.push(node.folders[child]);
              $scope.alreadySelected = false;  
            }else{          
              $scope.alreadySelected = true;
            } 
          }
        }       
      }
    };

    $scope.removeDevice = function(device){
      deviceIndex = $scope.newDevicesToAdd.indexOf(device);
      if(deviceIndex!==-1){
        device.show = false;
      }else{
        deviceIndex = $scope.permittedDevices.indexOf(device);
        $scope.permittedDevices.splice(deviceIndex,1);
        $scope.devicesToRemove.push(device);
      }
    };

    $scope.setPermissions = function(){
      // Iterate to set new permissions
      for(device in $scope.newDevicesToAdd){
        if($scope.newDevicesToAdd[device].show){
          $scope.addPermissionPost = $http.get('api/user/' + $scope.username + '/' + $scope.newDevicesToAdd[device].name).
            success(function(response){
              if(response.error){
                console.log("Some errors occurred. " + response.message);
              }
            }).error(function(response){
              console.log(response);
            });
        }
      }

      // Iterate to remove current permissions
      for(device in $scope.devicesToRemove){
        $scope.removePermissionPost = $http.delete('api/user/' + $scope.username + '/' + $scope.devicesToRemove[device].name).
          success(function(response){
            if(response.error){
              console.log("Some errors occurred. " + response.message);
            }
          }).error(function(response){
            console.log(response);
          });
      }
      $modalInstance.dismiss();      
    };

    $scope.cancel = function(){
      $modalInstance.dismiss();
    };
  });
  
  /**
   *Controller to manage user password change
   *@param object $scope scope for this user
   *@param service $http service instance to perform http requests
   *@param object $modalInstance modal passed to controller
   *@param service Alert service to manage messages to user 
   *@param object $stateParams parameters passed through state 
   */
  app.controller('ChangePasswordCtrl', function($scope,$http,$modalInstance,Alert,$stateParams){
    /**
     * Sends an email to the user to change password
     */
    $scope.sendRecoveryMail = function(username){
      if(username.trim!=''){
        $scope.emailPost = $http.post('api/user/'+username+'/send_recovery')
          .success(function(response){
            if(response.error){
              console.log("Some errors occurred. " + response.message);
            }else{
              $modalInstance.dismiss();
              Alert.open('success', response.message );
            }
          }).error(function(response){
            console.log(response);
          });          
      }
    }

    $scope.cancel = function(){
      $modalInstance.dismiss();
    }    
  }); 

  app.controller('ResetPasswordCtrl', function($rootScope,$scope,$http,Alert,$stateParams,$state){
    $scope.user = {};
    $scope.user.token = $stateParams.token;
    
    $scope.resetPassword = function(){
      $scope.changePassword = $http.put('api/user/' + $stateParams.user + '/password_recovery',
        {password: $scope.user.password, password_confirm: $scope.user.password_confirm,token:$scope.user.token})
        .success(function(response){
          if(response.error){
            console.log('Some errors occurred. ' + response.message);
          }else{
            $state.go('login');
            Alert.open('success','Your password was successfully changed.');
          }
        }).error(function(response){
            console.log(response);
        });
    }
  });   
})();