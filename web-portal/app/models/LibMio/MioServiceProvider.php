<?php namespace LibMio;
use Illuminate\Support\ServiceProvider;

class MioServiceProvider extends ServiceProvider{
  /**
   * Register service provider, this allowes us to use our facade
   * @return void returns nothing.
   */
  public function register(){
    $this->app['mio'] = $this->app->share(function($app){
        return new Mio;
    });

    $this->app->booting(function(){
        $loader = \Illuminate\Foundation\AliasLoader::getInstance();
        $loader->alias('Mio','LibMio\Facades\MioFacade');
    });
  }

}