(function(){
  /*
  * passwordMatch directive to evaluate two passwords matching
  */
  var app = angular.module('password-match-directive',[]);
  app.directive('passwordMatch', function() {
    return {
      restrict: 'A',
      scope:true,
      require: 'ngModel',
      link: function (scope, elem , attrs,control){
        var checker = function () {
        //get the value of the first password
        var password = scope.$eval(attrs.ngModel);

        //get the value of the second password  
        var password2 = scope.$eval(attrs.passwordMatch);
        return password == password2;
      };
        scope.$watch(checker, function (n) {
          //set the form control to valid if both 
          //passwords are the same, else invalid
          control.$setValidity("unique", n);
        });
      }
    };
  }); 
})();