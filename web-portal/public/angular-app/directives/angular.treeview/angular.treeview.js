/*
	@license Angular Treeview version 0.1.6
	ⓒ 2013 AHN JAE-HA http://github.com/eu81273/angular.treeview
	License: MIT


	[TREE attribute]
	angular-treeview: the treeview directive
	tree-id : each tree's unique id.
	tree-model : the tree model on $scope.
	node-id : each node's id
	node-label : each node's label
	node-children: each node's children
	data-node-filter: filter to search


	<div
		data-angular-treeview="true"
		data-tree-id="tree"
		data-tree-model="roleList"
		data-node-id="roleId"
		data-node-label="roleName"
		data-node-children="children" 
		data-node-filter="filter"
		node-callback="select"
		collection-callback="select">
	</div>
*/

(function ( angular ) {
	'use strict';

	angular.module( 'angularTreeview', [] ).directive( 'treeView', ['$compile','$http','$state','Folder', function( $compile,$http,$state,Folder ) {
		
		return {
			restrict: 'A',
			scope:{
				nodeCallback : '=',
				collectionCallback : '=',
				treeId : '=',
				hideRoot : '=?',
				folders:'=?',
				textFilter:'=?'
			},
			link: function ( scope, element, attrs) {

				//tree id
				var treeId = 'treeId';
				//tree model
				var treeModel = attrs.treeModel || 'folders';
				// array of id for instance of folders
				if(typeof scope.folders === 'undefined'){
					scope.folders=Folder.children;
				}
				

				//tree references
				var treeReferences = 'references';
				// array list of instance of folder
    		scope.references=Folder.references;
				//node id
				var nodeId = 'id';

				//node label
				var nodeLabel =  'name';

				//children
				var nodeChildren =  'children';

				//node type
				var nodeType =  'type';

				//search form
				var treeShowSearch = attrs.treeShowSearch ;

				//Node callback
				var nodeCallback = attrs.nodeCallback || 'select';
				//Collection callback
				var collectionCallback = attrs.collectionCallback || 'select';
				
				//Don't show Device in browser
				var hideDevice = attrs.hideDevice || false;

				//Don't show root
				var hideRoot = false;
				if(typeof attrs.hideRoot !== 'undefined'){
					var hideRoot = scope.hideRoot;
				}
				//Expand all who's have childrens
				var expandAll=attrs.expandAll || false;

				//Don't show favorites
				var hideFavorite = attrs.hideFavorite || false;

				//Add folder
				var addFolder = attrs.addFolder || true;
				//Know if is from a modal
				var fromModal= attrs.fromModal || false;
				var parameter =(fromModal)?'({modal:\''+fromModal+'\'})':'({device:\'\'})';
				//tree template
				var template =
					// '<div data-ng-show="'+treeShowSearch+'">Search: <input ng-model="textFilter" /></div>'+
					'<ul>' +
						'<li ng-hide="'+addFolder+'">'+
							'<a class="btn btn-link btn-sm" style="padding-left:0;" ui-sref="'+addFolder+parameter+'"><span class="glyphicon glyphicon-plus-sign"></span> Add Folder</a>' +
						'</li>'+
						'<li ng-hide="'+treeReferences+'[node].'+nodeId+'==\'root\' && '+hideRoot+' || '+treeReferences+'[node].'+nodeLabel+'==\'Favorites\' && '+hideFavorite+'" data-ng-repeat="node in ' + treeModel +' | filter:textFilter track by $index">' +
							'<i class="collapsed" data-ng-show="'+treeReferences+'[node].'+nodeType+'==\'location\' && (('+expandAll+')?'+treeId+'.expanded['+treeReferences+'[node].'+nodeId+']:!'+treeId+'.expanded['+treeReferences+'[node].'+nodeId+'])" data-ng-click="' + treeId + '.selectNodeHead('+treeReferences+'[node])"></i>' +
							'<i class="expanded" data-ng-show="'+treeReferences+'[node].'+nodeType+'==\'location\' && (('+expandAll+')?!'+treeId+'.expanded['+treeReferences+'[node].'+nodeId+']:'+treeId+'.expanded['+treeReferences+'[node].'+nodeId+'])" data-ng-click="' + treeId + '.selectNodeHead('+treeReferences+'[node])"></i>' +
							'<i class="normal" data-ng-show="'+treeReferences+'[node].'+nodeType+'==\'device\' && !'+hideDevice+'" ></i> ' +
							'<span data-ng-class="'+treeId+'.selected['+treeReferences+'[node].'+nodeId+']" ng-hide="'+treeReferences+'[node].'+nodeType+'==\'device\' && '+hideDevice+'" data-ng-click="' + treeId + '.selectNodeLabel('+treeReferences+'[node])">{{'+treeReferences+'[node].' + nodeLabel + '}}</span>' +
							'<div data-tree-view="false" data-ng-hide="('+expandAll+')?'+treeId+'.expanded['+treeReferences+'[node].'+nodeId+']:!'+treeId+'.expanded['+treeReferences+'[node].'+nodeId+'] " data-tree-show-search="false" data-tree-id="' + treeId + '" data-folders="'+treeReferences+'[node].' + nodeChildren + '" data-tree-model="folders"   data-node-callback="nodeCallback" data-collection-callback="collectionCallback" data-hide-device="'+hideDevice+'" data-expand-all="'+expandAll+'" data-text-filter="textFilter"></div>' +
						'</li>' +
					'</ul>';


					
				//check tree id, 
				if( scope.treeId  ) {
					//root node
					if( attrs.treeView == 'true') {
					
						//create tree object if not exists
						scope.treeId = scope.treeId || {};
						//Propertys to manage the directive
						scope.treeId.expanded ={};
						scope.treeId.selected ={};						
						//if node head clicks,
						scope.treeId.selectNodeHead = scope.treeId.selectNodeHead || function( selectedNode ){
							//Collapse or Expand	
							if(!scope.treeId.expanded[selectedNode.id] && selectedNode[nodeChildren].length <= 0){
								selectedNode.getChildren();
							}
							scope.treeId.expanded[selectedNode.id] = !scope.treeId.expanded[selectedNode.id];
						};

						//if node label clicks,
						scope.treeId.selectNodeLabel = scope.treeId.selectNodeLabel || function( selectedNode ){
							
							//remove highlight from previous node
							if( scope.treeId.currentNode && scope.treeId.selected[scope.treeId.currentNode.id] ) {
								scope.treeId.selected[scope.treeId.currentNode.id] = undefined;
							}

							//set highlight to selected node
							scope.treeId.selected[selectedNode.id] = 'selected';
							
							//set currentNode
							scope.treeId.currentNode = selectedNode;
							if(selectedNode[nodeType] == 'location' ){
								scope.collectionCallback(scope.treeId.currentNode);
								var isExpanded = (expandAll)?scope.treeId.expanded[selectedNode.id]:!scope.treeId.expanded[selectedNode.id];
								if(isExpanded){
									scope.treeId.selectNodeHead(scope.treeId.currentNode);
								}
							}else{
								scope.nodeCallback(scope.treeId.currentNode);
							}
						};
					}
					
					//Select 
					scope.$watch(function(){return scope.treeId.currentNode;},function(newValue, oldValue){	      		
	      		if($state.is('device.list',{folder:'root'})){
 	      			Folder.getChildren();
							scope.treeId.selectNodeLabel(Folder.references['root']); 
	      		}else if(typeof scope.treeId.currentNode == 'undefined' && !$state.is('device.list') && !$state.includes('device.view')){
 	      			Folder.getChildren();
							scope.treeId.selectNodeLabel(Folder.references['root']);
		     		}else if(typeof scope.treeId.currentNode == 'undefined' && $state.includes('device.view')){
		     			scope.treeId.selectNodeHead(Folder.references['root']);
 	      		}
			    });
						
					
					//Rendering template.
					element.html('').append( $compile( template )( scope ) );
				}
			}
		};
	}]);
})( angular );
