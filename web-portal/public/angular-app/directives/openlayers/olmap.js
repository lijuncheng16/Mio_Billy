(function() {
    var app = angular.module('olmap-directive', ['alert-handler','cgBusy']);

    /**
     * Directive to show devices in a map
     */
    app.directive('olmap', function($http,$state,$q,$timeout,Alert) {
      return {
          restrict: 'A',
          scope: {
            folderId:'=',
            deviceId:'=',
            showFolderName : '@',
            onClickCb:'&'
          },
          templateUrl: '/angular-app/directives/openlayers/olmap.html',
          link: function(scope, element, attrs) {
            var map;
            scope.device = {};
            scope.devices = [];
            var initMap = function(){
              if(scope.mapImg ==''){
                return;
              }
              var img = new Image();
              img.onload = function() {
                var intMapWidth = this.naturalWidth;
                var intMapHeight = this.naturalHeight;
                var pixelProjection = new ol.proj.Projection({
                  code: 'pixel',
                  units: 'pixels',
                  extent: [0, 0, intMapWidth, intMapHeight]
                });
                var imageLayer = new ol.layer.Image({
                      source: new ol.source.ImageStatic({
                        url: scope.mapImg,
                        imageSize: [intMapWidth, intMapHeight],
                        projection: pixelProjection,
                        imageExtent: pixelProjection.getExtent()
                      })
                    });
                var iconStyle = new ol.style.Style({
                  image: new ol.style.Icon({
                    anchor: [0.5,51],
                    anchorXUnits: 'fraction',
                    anchorYUnits: 'pixels',
                    opacity: 0.75,
                    src: '/img/marker.png'
                  })
                });
                var features = [];
                for(var i in scope.devices){
                  var device = scope.devices[i];
                  if(typeof device.location !='undefined'){
                    var feature = new ol.Feature({
                      geometry: new ol.geom.Point([device.location.lat, device.location.lon]),
                      id: device.id,
                      name: device.name,
                      info: device.info,
                    });
                    feature.setStyle(iconStyle);
                    features.push(feature);
                  }
                }
                var vectorLayer = new ol.layer.Vector({
                  source: new ol.source.Vector({
                    features: features
                  })
                });
                map = new ol.Map({
                  layers: [imageLayer,vectorLayer],
                  target: 'the-ol-map',
                  view: new ol.View({
                    projection: pixelProjection,
                    center: ol.extent.getCenter(pixelProjection.getExtent()),
                    zoom: 2
                  })
                });
                var vectorExtent = vectorLayer.getSource().getExtent();
                //extent is returned as string... so lets turn them to ints
                var numVectorExtent = [];
                for(var i in vectorExtent){
                  numVectorExtent.push(vectorExtent[i]*1);
                }
                var center = ol.extent.getCenter(numVectorExtent);
                if(features.length){
                  map.getView().setCenter(center);
                }
                var element = document.getElementById('popup');
                var popup = new ol.Overlay({
                  element: element,
                  positioning: 'bottom-center',
                  stopEvent: true
                });
                map.addOverlay(popup);

                // display popup on click
                map.on('click', function(evt) {
                  if(typeof attrs.onClickCb != 'undefined'){
                    var feature = new ol.Feature({
                      geometry: new ol.geom.Point([evt.coordinate[0],evt.coordinate[1]]),
                      id: device.id,
                      name: device.name,
                      info: device.info,
                    });
                    feature.setStyle(iconStyle);
                    map.removeLayer(vectorLayer);
                    vectorLayer = new ol.layer.Vector({
                      source: new ol.source.Vector({
                        features: [feature]
                      })
                    });
                    map.addLayer(vectorLayer);
                    scope.onClickCb({lon:evt.coordinate[1],lat:evt.coordinate[0]});
                    return;
                  }
                  var feature = map.forEachFeatureAtPixel(evt.pixel,
                      function(feature, layer) {
                        return feature;
                      });
                  if (feature) {
                    var geometry = feature.getGeometry();
                    var coord = geometry.getCoordinates();
                    popup.setPosition(coord);
                    $timeout(function(){
                      scope.device = {
                        id: feature.get('id'),
                        name:feature.get('name'),
                        info:feature.get('info')
                      }
                    });
                    angular.element(element).show();
                  } else {
                    angular.element(element).hide();
                  }
                });
                angular.element(map.getViewport()).on('mousemove', function(e) {
                  var pixel = map.getEventPixel(e.originalEvent);
                  var hit = map.forEachFeatureAtPixel(pixel, function(feature, layer) {
                    return true;
                  });
                  if (hit) {
                    angular.element('#'+map.getTarget()).css('cursor','pointer');
                  } else {
                    angular.element('#'+map.getTarget()).css('cursor','');
                  }
                });
              };
              img.src = scope.mapImg;
            };
            var initDirective = function(){
              scope.folderLoading = $http.get('/api/device/'+scope.folderId).success(function(response){
                if(response.error){
                  // $state.go('device.list');
                  Alert.open('warning',response.message);
                  return;
                }
                if(typeof response.device.properties =='undefined' || typeof response.device.properties.mapUri =='undefined'){
                  // $state.go('device.list');
                  Alert.open('warning','This folder does not have a map');
                  return;
                }
                scope.folder = response.device;
                scope.mapImg = response.device.properties.mapUri;
                if(typeof scope.deviceId != 'undefined'){
                  scope.folderLoading = $http.get('/api/device/'+scope.deviceId).success(function(response){
                    scope.devices.push(response.device);
                    initMap();
                  });
                }else{
                  scope.folderLoading = $http.get('/api/folder/'+scope.folderId+'/get_devices')
                    .success(function(response){
                      if(response.error){
                        Alert.open('warning',response.message);
                        return;
                      }
                      if(response.children.length < 1){
                        initMap();
                        // Alert.open('warning','This folder does not contain devices');
                        return;
                      }
                      var deviceRequests = [];
                      for(var i in response.children){
                        var id = response.children[i].id;
                        var req = $http.get('/api/device/'+id).success(function(response){
                          if(!response.error){
                            scope.devices.push(response.device);
                          }
                        });
                        deviceRequests.push(req);
                      }
                      $q.all(deviceRequests).then(function(){
                        initMap();
                      });
                  });
                }
              });
            };
            scope.$watch('folderId',function(n,o){
              if(typeof n !=='undefined'){
                initDirective();
              }
            });
          }
        };
    });
})();