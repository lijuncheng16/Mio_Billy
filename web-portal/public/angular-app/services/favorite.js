(function(){
  var app=angular.module('favorite-service',['favorite-service']);

  app.service('Favorite',function(){
    this.id=null;
    this.name=null;
  });
})();