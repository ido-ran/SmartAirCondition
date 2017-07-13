#!/usr/bin/env python

# Copyright 2016 Google Inc.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

# [START imports]
import os
import urllib

from google.appengine.api import users
from google.appengine.ext import ndb

import jinja2
import webapp2

JINJA_ENVIRONMENT = jinja2.Environment(
    loader=jinja2.FileSystemLoader(os.path.dirname(__file__)),
    extensions=['jinja2.ext.autoescape'],
    autoescape=True)
# [END imports]

DEFAULT_LOCATION = 'livingroom'


# We set a parent key on the 'Greetings' to ensure that they are all
# in the same entity group. Queries across the single entity group
# will be consistent. However, the write rate should be limited to
# ~1/second.

def aircondition_key(location=DEFAULT_LOCATION):
    """Constructs a Datastore key for a AirCondition entity.

    We use guestbook_name as the key.
    """
    return ndb.Key('AirCondition', location)


# [START DB classes]
class UpdateBy(ndb.Model):
    """Sub model for representing an author."""
    identity = ndb.StringProperty(indexed=False)
    email = ndb.StringProperty(indexed=False)

class IRFunction(ndb.Model):
    """A InfraRed function the user can perform"""
    name = ndb.StringProperty(indexed=False)
    ir_code = ndb.StringProperty(indexed=False)
    visual_type = ndb.StringProperty(indexed=False)
    display_order = ndb.IntegerProperty(indexed=True)

class AirConditionOp(ndb.Model):
    """A main model for representing an individual Guestbook entry."""
    update_by = ndb.StructuredProperty(UpdateBy)
    state = ndb.IntegerProperty(indexed=False)
    date = ndb.DateTimeProperty(auto_now=True)
    ir_func_name = ndb.StringProperty(indexed=False)
    ir_func_key = ndb.KeyProperty(kind=IRFunction)
# [END DB classes]


# [START main_page]
class MainPage(webapp2.RequestHandler):

    def get(self):
        location = DEFAULT_LOCATION
        ops_query = AirConditionOp.query(
            ancestor=aircondition_key()).order(-AirConditionOp.date)
        ops = ops_query.fetch(10)

        ir_funcs_query = IRFunction.query(
            ancestor=aircondition_key()).order(IRFunction.display_order)
        ir_funcs = ir_funcs_query.fetch()

        user = users.get_current_user()
        if user:
            url = users.create_logout_url(self.request.uri)
            url_linktext = 'Logout'
        else:
            url = users.create_login_url(self.request.uri)
            url_linktext = 'Login'

        template_values = {
            'user': user,
            'ir_funcs': ir_funcs,
            'ops': ops,
            'url': url,
            'url_linktext': url_linktext,
        }

        template = JINJA_ENVIRONMENT.get_template('index.html')
        self.response.write(template.render(template_values))
# [END main_page]


# [START guestbook]
class DoOperation(webapp2.RequestHandler):

    def post(self):
        airconditionop = AirConditionOp(parent=aircondition_key())

        ir_func_key = ndb.Key(urlsafe=self.request.get('ir_func'))
        ir_func = ir_func_key.get()

        airconditionop.update_by = UpdateBy(
                identity=users.get_current_user().user_id(),
                email=users.get_current_user().email())

        airconditionop.ir_func_name = ir_func.name
        airconditionop.ir_func_key = ir_func_key

        airconditionop.put()

        self.redirect('/')
# [END guestbook]

class Api(webapp2.RequestHandler):

    def get(self):
        location = DEFAULT_LOCATION
        lastop_query = AirConditionOp.query(
            ancestor=aircondition_key(location)).order(-AirConditionOp.date)
        last_op = lastop_query.get()
        ir_func = last_op.ir_func_key.get()

        self.response.write(last_op.date.strftime('%S%f'))
        self.response.write('\r\n')
        self.response.write(ir_func.ir_code)

class AddIRFunc(webapp2.RequestHandler):

    def post(self):
        ir_func = IRFunction(
            parent=aircondition_key(),
            name = self.request.get('ir_func_name'),
            ir_code = self.request.get('ir_code'),
            visual_type = self.request.get('visual_type'),
            display_order = int(self.request.get('display_order'))
        )

        ir_func.put()

        self.redirect('/')

def error_handler_middleware(app):
    """Wraps the application to catch uncaught exceptions."""
    def wsgi_app(environ, start_response):
        try:
            return app(environ, start_response)
        except Exception, e:
            logging.exception(e)
            # ... display a custom error message ...
            response = webapp.Response()
            response.set_status(500)
            response.out.write('Ooops! An error occurred...')
            response.wsgi_write(start_response)
            return ['']

    return wsgi_app

# [START app]
app = webapp2.WSGIApplication([
    ('/', MainPage),
    ('/do', DoOperation),
    ('/add_ir_func', AddIRFunc),
    ('/api', Api),
], debug=True)

app = error_handler_middleware(app)

# [END app]
