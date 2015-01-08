<?php

class HomeController extends BaseController {

	/*
	|--------------------------------------------------------------------------
	| Default Home Controller
	|--------------------------------------------------------------------------
	|
	| You may wish to use controllers instead of, or in addition to, Closure
	| based routes. That's great! Here is an example controller method to
	| get you started. To route to this controller, just add the route:
	|
	|	Route::get('/', 'HomeController@showWelcome');
	|
	*/
	/**
	 * Welcoming screen, loads the angular app
	 * @return angular view.
	 */
	public function angular() {
		return View::make('angular');
	}
	/**
	 * Password recovery, loads the angular app, but sets a variable to be used.
	 * @return Angular view
	 */
	public function passwordRecovery($strUsername,$strToken){
		return View::make('angular',array('username'=>$strUsername,'token'=>$strToken));
	}

}
