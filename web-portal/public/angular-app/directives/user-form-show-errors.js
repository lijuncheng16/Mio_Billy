(function(){
  /*
  * showErrors directive to validate and display errors on user edit and create forms
  */

  var app = angular.module('show-errors-directive',[]);
  app.directive('showErrors', function() {
    return {
      restrict: 'A',
      require:  '^form',
      link: function (scope, el, attrs, formCtrl) {
        var inputEl   = el[0].querySelector('input[type=text],input[type=password],input[type=email],select');
        var inputNgEl = angular.element(inputEl);
        var inputName = inputNgEl.attr('name');

        // only apply the has-error class after the user leaves the text box
        inputNgEl.on('blur', function() {
          el.toggleClass('has-error', formCtrl[inputName].$invalid);
        });
      }
    }
  });
})();