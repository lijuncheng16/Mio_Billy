(function(){ 
  /**
   * Min directive to set the min value permitte in an input
   * @type {[type]}
   */
  var app=angular.module('min-directive',[]);
  
  function isEmpty(value) {
    return angular.isUndefined(value) || value === '' || value === null || value !== value;
  }
  app.directive('ngMin', function() {
      return {
        restrict: 'A',
        require: 'ngModel',
        link: function(scope, elem, attr, ctrl) {
          scope.$watch(attr.ngMin, function(){
              ctrl.$setViewValue(ctrl.$viewValue);
          });
          var minValidator = function(value) {
            var min = scope.$eval(attr.ngMin) || 0;
            if (!isEmpty(value) && value < min) {
              ctrl.$setValidity('ngMin', false);
              return undefined;
            } else {
              ctrl.$setValidity('ngMin', true);
              return value;
            }
          };

          ctrl.$parsers.push(minValidator);
          ctrl.$formatters.push(minValidator);
        }
      };
  });
})();

