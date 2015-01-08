      
		//urlDomain = '';
		//urlDomain = 'http://172.16.66.129:8080';
      //urlDomain = 'http://sensor2.andrew.cmu.edu';

		
		function loadJson(url, successCallback, failureCallback, completeCallback, willCache)
         {
         willCache = (typeof willCache === 'undefined') ? true : willCache;

         var handleFailure = function(jqXHR, textStatus, errorThrown)
            {
            // TODO: do something better here...
            console.log("loadJson.handleFailure(): FAILURE! errorThrown:" + errorThrown);
            try
               {
               if (typeof failureCallback === 'function')
                  {
                  failureCallback(errorThrown);
                  }
               }
            catch (ex)
               {
               // TODO: do something better here...
               console.log("loadJson.handleFailure(): FAILURE! ex:" + ex);
               }
            };
         $.ajax(
               {
                  cache:willCache,
                  url:url,
                  success:function(data, textStatus, jqXHR)
                     {
                     try
                        {
                        if (typeof successCallback === 'function')
                           {
                           // send the JSON as a String...
                           successCallback(typeof data === 'string' ? data : JSON.stringify(data));
                           }
                        }
                     catch (ex)
                        {
                        handleFailure(jqXHR, "JSON parse error", ex);
                        }
                     },
                  error:handleFailure,
                  complete:function(jqXHR, textStatus)
                     {
                     try
                        {
                        if (typeof completeCallback === 'function')
                           {
                           completeCallback(jqXHR, textStatus);
                           }
                        }
                     catch (ex)
                        {
                        // TODO: do something better here...
                        console.log("loadJson(): Failed to call the completeCallback:" + ex);
                        }
                     }
               }
            );
         }

      function channelDatasource(userId, deviceName, channelName)
         {
         var urlPrefix = "/tiles/" + userId + "/" + deviceName + "." + channelName + "/";

         return function(level, offset, successCallback, failureCallback)
            {
				var url = urlDomains[0] + urlPrefix + level + "." + offset + ".json";
				console.log("channelDatasource(" + url + ")");
            loadJson(url, successCallback, failureCallback, null, true);
            }
         }

   
      function createPlot(deviceName, channelName, style, willShowComments, dateAxis, yAxisElementId, yAxisRange)
         {
         yAxisRange = yAxisRange || {"min":26, "max":46};
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

      function displayValue1(val)
         {
         v1 = (val == null ? "" : val['dateString'] + " " + val['valueString']);
         displayComboValue();
         }

      function displayValue2(val)
         {
         v2 = (val == null ? "" : val['dateString'] + " " + val['valueString']);
         displayComboValue();
         }

      function displayComboValue()
         {
         var val = v1 + (v1 != "" && v2 != "" ? " | " : "") + v2;
         if (val)
            {
            $("#valueLabel").html(val);
            }
         else
            {
            $("#valueLabel").html("");
            }
         }

      function displayValue(val)
         {
         if (val)
            {
            $("#valueLabel").html(val['dateString'] + " " + val['valueString']);
            }
         else
            {
            $("#valueLabel").html("");
            }
         }


