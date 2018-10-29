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

        RPC : null,

        install : function() {
        	user.addEvent(window, 'load', user.init);
        },

        init : function() 
	{
		var el;

		if(user.binding) {
                        user.bind();
                }

		user.rpc_init(user.rpc, true);

		el = document.getElementById('button-signin');
                if(el)
                {
                        el.onclick = function(e) {
                                user.rpc_signin();
                                return false;
                        }
                }
                el = document.getElementById('button-signout');
                if(el)
                {
                        el.onclick = function(e)
                        {
                                user.rpc_signout();
                                return false;
                        }
                }
        },
        bind : function() {
		//var e = document.getElementById(bindId);
	},

        addEvent: function(el, evnt, func) {
		if(el.addEventListener) {
			el.addEventListener(evnt, func, false);
                } else if(el.attachEvent) {
                        el.attachEvent('on'+evnt, func);
                }
        },

	show_alert: function(el, text) {
		el.innerHTML = text;
		el.style.display = "block";
	},

	hide_alert: function(el) {
		el.style.display = "none";
	},

        rpc_init: function(res, async) {
                try {
                        user.RPC = new JsonRpc.ServiceProxy(res, {
                                asynchronous: true,  //default value, but if otherwise error raised
                                sanitize: false,     //explicit false required, otherwise error raised
				methods: ['signup','signin','signout'],
				callbackParamName: 'callback'
                        });
                }
                catch(e){
                        alert('Please, add `json-xml-rpc` JavaScript!');
                }
        },

        rpc_signup: function() {
                var email = document.getElementById('signup-email').value;
		var name = document.getElementById('signup-name').value;
                var password = document.getElementById('signup-password').value;
		var confirm_password = document.getElementById('signup-confirm-password').value;
		var signup_alert = document.getElementById('signup-alert');

		if( password != confirm_password) {
			show_alert(signup_alert, "The confirm password should match password");
		}

                JsonRpc.setAsynchronous(user.RPC, true);
                user.RPC.signup({params:[ email, password, name ],
                        onSuccess:function(resultObj)
                        {
				window.location.href = '/u/verify';
                        },
                        onException:function(errorObj)
                        {
				show_alert(signup_alert, errorObj.error);
				
                        },
                        onComplete:function(responseObj)
                        {
                                if(responseObj && responseObj.error == null && responseObj.result == "ok") {
                                        if(window.location.href.indexOf("#")>0)
                                                window.location.href = window.location.href.split("#")[0];
                                        setTimeout(location.reload(true), 1000);
                                }
                                else
					show_alert(signup_alert, errorObj.error);
                        }
                });
        },

	rpc_signin: function() {
                var email = document.getElementById('signin-email').value;
                var password = document.getElementById('signin-password').value;

                JsonRpc.setAsynchronous(user.RPC, true);
                user.RPC.signin({params:[ email, password ],
                        onSuccess:function(resultObj)
                        {
				window.location.href = '/u/profile';
                        },
                        onException:function(errorObj)
                        {
				show_alert(signup_alert, errorObj.error);
                        },
                        onComplete:function(responseObj)
                        {
                                if(responseObj && responseObj.error == null && responseObj.result == "ok") {
                                        if(window.location.href.indexOf("#")>0)
                                                window.location.href = window.location.href.split("#")[0];
                                        setTimeout(location.reload(true), 1000);
                                }
                                else
                                	show_alert(signin_alert, errorObj.error);
                        }
                });
        },

	rpc_signout: function() {
                JsonRpc.setAsynchronous(user.RPC, true);
                user.RPC.signout({params:[ ],
                        onException:function(errorObj){
                                alert("Exception: " + errorObj);
                        },
                        onComplete:function(responseObj){
                                if(responseObj && responseObj.error == null && responseObj.result == "ok") {
                                        if(window.location.href.indexOf("#")>0)
                                                window.location.href = window.location.href.split("#")[0];
                                        setTimeout(location.reload(true),1000);
                                }
                        }
                });
        }
};

user.install();
