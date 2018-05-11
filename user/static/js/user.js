/**
 * user, JavaScript User Interface
 *
 * @version 0.0.1
 * @license GNU General Public License, http://www.gnu.org/licenses/gpl-3.0.html
 * @author  kpeo, opncms@gmail.com
 * @created 2015-05-15
 * @updated 2018-05-12
 * @link    http://opnpcms.com
 * @depend  -
 */

var user = {
        version : "0.0.1",
        rpc : "/u_rpc",
        binding : true, // automatic binding
        bindId : 'user', // class name
        preloading : true,

        dpRPC : null,

        install : function() {
        	user.addEvent(window, 'load', user.init);
        },

        init : function() 
	{
		if(user.binding) {
			user.bind();
		}
		var el = document.getElementById('button-singin');
		if( el != null ) {
			el.onmousedown = function(e) {
				var evt = window.event || e;
				return;
			}
		}
		user.rpc_init(user.rpc,1);
        },
        bind : function() {
		//var e = document.getElementById(bindId);
	},

        addEvent : function(el, evnt, func) {
		if(el.addEventListener) {
			el.addEventListener(evnt, func, false);
                } else if(el.attachEvent) {
                        el.attachEvent('on'+evnt, func);
                }
        },

        rpc_init: function(res, async) {
                try {
                        user.dpRPC = new rpc.ServiceProxy(res, {
                                asynchronous: (async ? true : false),  //default value, but if otherwise error raised
                                sanitize: false     //explicit false required, otherwise error raised
                        });
                }
                catch(e){
                        alert('Please, add `json-xml-rpc` JavaScript!');
                }
        },

};

user.install();
