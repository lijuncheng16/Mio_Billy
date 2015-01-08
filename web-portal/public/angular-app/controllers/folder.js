/**
 * Folder Controller module
 */
(function(){
  var app = angular.module('folder-controller',['ui.router','mortar-services','cgBusy','angularTreeview','ui.bootstrap','angularFileUpload']);
  /**
   * Controlles for managing folder components
   * @param  object $scope    scope of the controller
   * @param  factory User     User factory instance
   * @param  factory Folder   User factory instance
   * @param  factory Device   User factory instance
   * @parem  service $state   ui router state service
   * @param  service $http    angulars $http service
   * @param  filter $filter   angular filter service
   */
  app.controller('FolderCtrl',function($scope,User,Folder,Device,$state,$http,$filter){
    //Initial object of the controller
    $scope.user = User;
    $scope.selectedFolder={};
    $scope.devBrowser={};
    
    $scope.selectFolder = function(folder){
      $scope.selectedFolder=folder;
      $state.go('device.list',{folder:$scope.selectedFolder.id});
    }
    $scope.selectDevice = function(device){
      $state.go('device.view.detail',{id:device.id});
    };
  }); 

  app.controller('FolderCreateCtrl',function($scope,$modalInstance,$http,$state,$stateParams,$upload,$window,Alert,Folder,Favorite,User,Device){
    /**
     * Pass the folder name by post to create a folder and manage the responde  
     */
    $scope.user=User;
    $scope.favorite = Favorite;
    $scope.modalBrowser={};
    $scope.folder={
      folder_id:'',
      folder_name:'',
      folder_mapUri:'',
      folder_mapUriUrl:''
    };
    $scope.showRoot = (User.group==='admin') ? true : false;
    // $scope.folders=Folder.children;
    // $scope.references=Folder.references;
    $scope.isFromFavModal= typeof $stateParams.modal != 'undefined' && $stateParams.modal !== null;
    //$scope.isFromDevModal= typeof $stateParams.device != 'undefined' && $stateParams.device !== null && $stateParams.device != '';
    $scope.isRootOrFavorite = false;
    
    $scope.isUpdate=angular.isDefined($stateParams.folders) && $stateParams['folders'] != '' ;
    if($scope.isUpdate){
      //@todo edit function to get all data from folder
      $scope.loadFolder=$http.get('api/device/'+$stateParams['folders']).success(
        function(response){
          if(response['error']){
            Alert.open('warning',response.message);
          }
          $scope.folder.folder_id =response.device.id;
          $scope.folder.folder_name=response.device.name;
          if(typeof response.device.properties != 'undefined' && typeof response.device.properties.mapUri != 'undefined'){
            $scope.folder.folder_mapUri =  response.device.properties.mapUri;
          }else{
            $scope.folder.folder_mapUri =  '';
          }
          if(typeof $scope.folder.folder_mapUriUrl==='undefined'){
            $scope.folder.folder_mapUriUrl = '';
          }
          $scope.isRootOrFavorite =$scope.folder.folder_id == 'root' || $scope.folder.folder_id.indexOf('_favorites') != -1;
          if(typeof response.device.parent != 'undefined'){
            $scope.folder.parent=Folder.constructFolder(response.device.parent);
            $scope.folder.parent.children=[];
            $scope.modalBrowser.selectNodeLabel($scope.folder.parent);
          }
        }
      );
    }
    $scope.onFileSelect = function ($files){
      $scope.folder.file = $files[0];
    }
    /**
     * submit folder data
     */
    $scope.submitFolder=function(){ 
      if($scope.isUpdate){ //Check if is for update
        var url= '/api/folder/'+$stateParams['folders'];
        var data = {
          mapUri_url:$scope.folder.folder_mapUriUrl,
          folder_name:$scope.folder.folder_name,
          old_parent:($scope.isRootOrFavorite )?undefined:$scope.folder.parent.id,
          folder_parent:($scope.isRootOrFavorite)?undefined:$scope.selectedFolder.id
        };
        var success = function(response){
            if(response.error){
              Alert.open('warning','Folder could not be updated');
              return;
            }
            if($scope.folder.parent.id != $scope.selectedFolder.id){
              Folder.references[$scope.folder.parent.id].getChildren();
              $scope.selectedFolder.getChildren();
            }
            
            Alert.open('success','Folder updated');
              
            $modalInstance.close(true);
          };
        var error = function(response){
            Alert.open('danger',response);
          };
        if(typeof $scope.folder.file !='undefined'){
          $scope.savingPromise = $upload.upload({
            url:url,
            data:data,
            file:$scope.folder.file,
            fileFormDataName:'folder_mapUri'
          }).success(success).error(error);
        }else{
          $scope.savingPromise = $http.post(url,data).success(success).error(error);
        }
      }else{
        var url ='/api/folder/create_folder';
        var data = {
          mapUri_url:$scope.folder.folder_mapUriUrl,
          folder_name:$scope.folder.folder_name,
          folder_parent:$scope.selectedFolder.id
        };
        var success = function(response){
          if(response.error){
            Alert.open('warning','Could not create folder');
            return;
          }
          if($scope.isFromFavModal){
            $scope.favorite.id = response.folder_id;
            $scope.favorite.name = $scope.folder.folder_name;
            
          }
          $scope.user.getPermittedDevices();
          $scope.selectedFolder.getChildren();
          Alert.open('success','Folder created');
          $modalInstance.close(true);            
            
        };
        var error = function(response){
          Alert.open('danger',response);
        };
        if(typeof $scope.folder.file !='undefined'){
          $scope.savingPromise = $upload.upload({
            url:url,
            data:data,
            file:$scope.folder.file,
            fileFormDataName:'folder_mapUri'
          }).success(success).error(error);
        }else{
          $scope.savingPromise=$http.post(url,data).success(success).error(error);
        }
      }
    };
    $scope.selectFolder = function(folder){
      if(folder.type=='location'){
        $scope.selectedFolder=folder;
      }
    };
    $scope.selectDevice = function(device){

    };
    $scope.isNotSelect= function(){
      if($scope.isRootOrFavorite){
        return false;
      }
      return typeof $scope.selectedFolder == 'undefined';
    }
    /**
     * Close the create folder modal 
     */
    $scope.cancel = function () {     
      $modalInstance.dismiss();
    };    
  }); 
})();
