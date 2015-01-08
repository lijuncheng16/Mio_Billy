(function() {
  /**
   * Shows a timeseries chart for a list of transducers
   */
  var app = angular.module('timeseries-directive', ['alert-handler']);

  /**
   * Directive to show timeseries data, receives a list of transducers and a date range
   * @param  service $http http service
   * @param  factory Alert alert service, used to show alerts
   * @todo  should push new data to existing timeseries, instead of creating new one.
   */
  app.directive('timeseries', function($http, $timeout, Alert) {
      return {
          restrict: 'A',
          scope: {
            device: '=',
            toggleTransducers:'@'
          },
          templateUrl: '/angular-app/directives/timeseries/timeseries.html',
          link: function(scope, element, attrs) {
            scope.showTransducersToggle = typeof attrs.toggleTransducers != 'undefined';
            scope.togglePlot = function(device,transducer,isVisible){
              if(isVisible){
                angular.element('#'+device+transducer.split(' ').join('-')).hide();
              }else{
                angular.element('#'+device+transducer.split(' ').join('-')).show();
              }
            };
            var plot5Style = {
                "comments": {
                    "show": true,
                    "styles": [{
                        "show": true,
                        "type": "point",
                        "color": "#00ff00",
                        "radius": 2,
                        "lineWidth": 1,
                        "fill": true,
                        "fillColor": "red"
                    }, {
                        "show": true,
                        "type": "point",
                        "color": "#00ff00",
                        "radius": 4,
                        "lineWidth": 1,
                        "fill": false,
                        "fillColor": "green"
                    }]
                },
                "styles": [{
                    "type": "line",
                    "color": "#ff0000",
                    "lineWidth": 1,
                    "fill": true,
                    "fillColor": "#ff0000"
                }],
                "highlight": {
                    "lineWidth": 2,
                    "styles": [{
                        "type": "point",
                        "color": "#ff0000",
                        "radius": 3,
                        "lineWidth": 1,
                        "fill": true,
                        "fillColor": "blue"
                    }]
                }
            };
            var inDeviceTransducers = function(device,transducer){
              if(device != scope.device.id){
                return false;
              }
              for(var t in scope.device.transducers){
                if(scope.device.transducers[t].name.split(' ').join('-') == transducer){
                  return true;
                }
              }
              return false;
            };
            scope.graphErrors = false;
            scope.noGraphs = false;
            scope.gettingGraphs = true;
            urlDomains = {};
            var loadGraph = function() {
              var strDeviceId = scope.device.id;
              $http.get('/api/device/'+strDeviceId+'/get_storage_urls').success(function(response){
                if(response.error){
                  Alert.open('warning','Could not get the urls where the data is stored, please try again.');
                  scope.gettingGraphs = false;
                  scope.noGraphs = true;
                  return;
                }
                if(response.urls.length <= 0){
                  Alert.open('warning','Could not get the urls where the data is stored, please try again.');
                  scope.gettingGraphs = false;
                  scope.noGraphs = true;
                  return; 
                }
                urlDomains[strDeviceId] = response.urls;
                $.ajax({
                  url: urlDomains[strDeviceId][0]+'/info.json',
                  error: function(a,b,c){
                    $timeout(function(){
                      scope.graphErrors = true;
                      scope.gettingGraphs = false;
                    });
                  },
                  success: function(data, textStatus, jqXHR){
                    data = (typeof data === 'string') ? JSON.parse(data) : data;
                    var addedGraph = false;
                    var feeds = data['channel_specs'];
                    var plots = [];
                    var plotContainers = [];
                    var dateAxes = [];
                    var i = 0;
                    for (var key in feeds) {
                        var device = key.split('.')[0];
                        var chan = key.split('.')[1];
                        if (inDeviceTransducers(device,chan)) {
                            addedGraph =true;
                            element.append(' \
                           <div id="'+device+chan+'"> \
                           <h4>' + chan.split('-').join(' ') + '</h4> \
                           <table border="0" cellpadding="0" cellspacing="0" width="80%"> \
                           <tr> \
                           <td width="*%"> \
                           <div id="plotContainer' + (i + 1) + '" class="plotContainer"> \
                           <div id="pc' + (i + 1) + '" class="plot"></div> \
                           </div> \
                           </td> \
                           <td class="axisCell"> \
                           <div id="yAxisContainer' + (i + 1) + '" class="yAxisContainer"> \
                           <div id="yAxis' + (i + 1) + '" class="yAxis"></div> \
                           </div> \
                           </td> \
                           </tr> \
                           <tr> \
                           <td width="*%"> \
                           <div id="dateAxisContainer' + (i + 1) + '" class="dateAxisContainer"> \
                           <div id="dateAxis' + (i + 1) + '" class="dateAxis"></div> \
                           </div> \
                           </td> \
                           </tr> \
                           </table> \
                           </div> \
                           ');
                            dateAxes[i] = new DateAxis("dateAxis" + (i + 1), "horizontal", {
                                "min": feeds[key]['channel_bounds']['min_time'],
                                "max": feeds[key]['channel_bounds']['max_time']
                            });

                            plots[i] = createPlot(device, chan, plot5Style, true, dateAxes[i], "yAxis" + (i + 1), {
                                'min': feeds[key]['channel_bounds']['min_value'],
                                'max': feeds[key]['channel_bounds']['max_value']
                            });
                            plotContainers[i] = new PlotContainer("pc" + (i + 1), [plots[i]]);
                            i++;
                        }
                    }
                    $timeout(function(){
                      scope.gettingGraphs = false;
                      if(!addedGraph){
                        scope.noGraphs = true;
                      }
                    });
                  }
                });
              });
            };

            function loadJson(url, successCallback, failureCallback, completeCallback, willCache) {
                willCache = (typeof willCache === 'undefined') ? true : willCache;

                var handleFailure = function(jqXHR, textStatus, errorThrown) {
                    // TODO: do something better here...
                    // console.log("loadJson.handleFailure(): FAILURE! errorThrown:" + errorThrown);
                    try {
                        if (typeof failureCallback === 'function') {
                            failureCallback(errorThrown);
                        }
                    } catch (ex) {
                        // TODO: do something better here...
                        console.log("loadJson.handleFailure(): FAILURE! ex:" + ex);
                    }
                };
                $.ajax({
                    cache: willCache,
                    url: url,
                    success: function(data, textStatus, jqXHR) {
                        try {
                            if (typeof successCallback === 'function') {
                                // send the JSON as a String...
                                successCallback(typeof data === 'string' ? data : JSON.stringify(data));
                            }
                        } catch (ex) {
                            handleFailure(jqXHR, "JSON parse error", ex);
                        }
                    },
                    error: handleFailure,
                    complete: function(jqXHR, textStatus) {
                        try {
                            if (typeof completeCallback === 'function') {
                                completeCallback(jqXHR, textStatus);
                            }
                        } catch (ex) {
                            // TODO: do something better here...
                            console.log("loadJson(): Failed to call the completeCallback:" + ex);
                        }
                    }
                });
            }

            function channelDatasource(userId, deviceName, channelName) {
                var urlPrefix = "/tiles/" + userId + "/" + deviceName + "." + channelName + "/";

                return function(level, offset, successCallback, failureCallback) {
                    var url = urlDomains[scope.device.id][0] + urlPrefix + level + "." + offset + ".json";
                    // console.log("channelDatasource(" + url + ")");
                    loadJson(url, successCallback, failureCallback, null, true);
                }
            }


            function createPlot(deviceName, channelName, style, willShowComments, dateAxis, yAxisElementId, yAxisRange) {
                yAxisRange = yAxisRange || {
                    "min": 26,
                    "max": 46
                };
                var datasource = channelDatasource(1, deviceName, channelName);
                var yAxis = new NumberAxis(yAxisElementId, "vertical", yAxisRange);
                return new DataSeriesPlot(datasource, dateAxis, yAxis, style);
            }

            /*function createPhotoPlot(dateAxis, yAxisElementId)
               {
               var datasource = photoDatasource(1);
               var yAxis = new PhotoAxis(yAxisElementId, "vertical");
               return new PhotoSeriesPlot(datasource, dateAxis, yAxis, 1);
               }
            */

            var v1 = "";
            var v2 = "";

            function displayValue1(val) {
                v1 = (val == null ? "" : val['dateString'] + " " + val['valueString']);
                displayComboValue();
            }

            function displayValue2(val) {
                v2 = (val == null ? "" : val['dateString'] + " " + val['valueString']);
                displayComboValue();
            }

            function displayComboValue() {
                var val = v1 + (v1 != "" && v2 != "" ? " | " : "") + v2;
                if (val) {
                    $("#valueLabel").html(val);
                } else {
                    $("#valueLabel").html("");
                }
            }

            function displayValue(val) {
                if (val) {
                    $("#valueLabel").html(val['dateString'] + " " + val['valueString']);
                } else {
                    $("#valueLabel").html("");
                }
            }
            $timeout(function() {
              loadGraph();
            },500);
        }
      };
    });
})();