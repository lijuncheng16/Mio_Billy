/**
 * Folder Services
 */
(function(){
  var app=angular.module('folder-services',['user-services']);
  
  app.factory('Folder',function($http,$q,User,Alert){
    var FolderService={};
    /**
    * Construct function 
    */
    function Folder(json){
      angular.extend(this, json);
    }
    
    Folder.prototype={
      /**
      * @param folderId id of folder  
      */
      getDevicesByFolder : function(folderId){
        return $http.get('/api/folder/'+folderId+'/get_devices')
        .success(function(response){
          if(response.error){
            Alert.open('warning','Could not retrieve the devices in ' + FolderService.references[folderId].name+', try refreshing the page.');
            return;
          }
          return response.objects; 
        })
        .error(function(response){
          console.log('something wrong with the server');
          console.log(response);
        });
      },
      /**
      * @param json object tha represent a folder  
      */
      addChildren : function(json){
        var self = this;
        var objFolder = new Folder(json);
        FolderService.references[objFolder.id] = objFolder;
        self.children.push(objFolder.id);
      },
      /**
      * @return children of a folder
      */
      getChildren : function(){
        var self =this;
        $http.get('/api/folder/'+self.id+'/get_children')
        .success(function(response){
          if(response.error){
            Alert.open('warning','Could not retrieve the children of ' + FolderService.references[self.id].name + ', please try expanding the folder again.');
            return ;
          }
          angular.copy([],self.children);
          
          for(folder in response.children){
            if(typeof self.isFavorite !== 'undefined'){
              response.children[folder].isFavorite=self.isFavorite;
            }
            if(typeof FolderService.references[response.children[folder].id] != 'undefined' && FolderService.references[response.children[folder].id].children.length > 0 ){
              response.children[folder].children = FolderService.references[response.children[folder].id].children;
            }else{
              response.children[folder].children=[];
            }
            var objFolder=new Folder(response.children[folder]); 
            FolderService.references[objFolder.id] = objFolder; 
            self.children.push(objFolder.id);
          } 
        }).error(function(response){
          console.log('something wrong with the server');
          console.log(response);
        });
      }
    };
    
    
    /**
    * @param json object to represent folder to construct
    */
    FolderService.constructFolder= function(json){
      return new Folder(json);
    };
    /**
    * Init Folder service
    */
    FolderService.init =function(){

      var userFavorite =User.username+'_favorites';
      
      /**
      * Children have first id of refrences for device broweser
      */
      FolderService.children=[
        'root',
        userFavorite
      ];
      /**
      * Folder service property references is a list of instance of folder
      */
      //Root instance
      FolderService.references = {
        root:new Folder({
          name:'root',
          id:'root',
          type:'location',
          relation:'',
          children:[]
        })
      };
      //Folder instance of favorite
      FolderService.references[userFavorite] = new Folder({
        name:'Favorites',
        id:userFavorite,
        type:'location',
        relation:'',
        isFavorite:true,
        children:[]
      });
    };
    FolderService.init();
    FolderService.deleteInstace =function(){
      FolderService={};
      FolderService.children=[];
    };
    /**
    *  Get first folder from root
    */
    FolderService.getChildren=function(){

      $http.get('/api/folder/root/get_children')
      .success(function(response){
        if(response.error){
          Alert.open('warning','Could not retrieve the children of the root folder, please try expanding the folder again.');
          return;
        }
        FolderService.references['root'].children = [];
        for(folder in response.children){
          if(typeof FolderService.references[response.children[folder].id] != 'undefined' && FolderService.references[response.children[folder].id].children.length > 0 ){
            response.children[folder].children = FolderService.references[response.children[folder].id].children;
          }else{
            response.children[folder].children=[];
          }
          var root =new Folder(response.children[folder]); 
          FolderService.references[root.id] = root;         
          FolderService.references['root'].children.push(root.id);
        } 
      }).error(function(response){
        console.log('something wrong with the server');
        console.log(response);
      });
    };

    
    return FolderService;
  });
  
})();