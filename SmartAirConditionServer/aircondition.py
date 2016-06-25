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


# [START greeting]
class UpdateBy(ndb.Model):
    """Sub model for representing an author."""
    identity = ndb.StringProperty(indexed=False)
    email = ndb.StringProperty(indexed=False)


class AirConditionOp(ndb.Model):
    """A main model for representing an individual Guestbook entry."""
    update_by = ndb.StructuredProperty(UpdateBy)
    state = ndb.IntegerProperty(indexed=False)
    date = ndb.DateTimeProperty(auto_now=True)
# [END greeting]


# [START main_page]
class MainPage(webapp2.RequestHandler):

    def get(self):
        location = DEFAULT_LOCATION
        ops_query = AirConditionOp.query(
            ancestor=aircondition_key(location)).order(-AirConditionOp.date)
        ops = ops_query.fetch(10)

        user = users.get_current_user()
        if user:
            url = users.create_logout_url(self.request.uri)
            url_linktext = 'Logout'
        else:
            url = users.create_login_url(self.request.uri)
            url_linktext = 'Login'

        template_values = {
            'user': user,
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
        # We set the same parent key on the 'Greeting' to ensure each
        # Greeting is in the same entity group. Queries across the
        # single entity group will be consistent. However, the write
        # rate to a single entity group should be limited to
        # ~1/second.
        airconditionop = AirConditionOp(parent=aircondition_key())

        airconditionop.update_by = UpdateBy(
                identity=users.get_current_user().user_id(),
                email=users.get_current_user().email())

        airconditionop.state = 1 if self.request.get('op') == 'on' else 0
        airconditionop.put()

        self.redirect('/')
# [END guestbook]

class Api(webapp2.RequestHandler):

    def get(self):
        location = DEFAULT_LOCATION
        lastop_query = AirConditionOp.query(
            ancestor=aircondition_key(location)).order(-AirConditionOp.date)
        last_op = lastop_query.get()
        self.response.write(str(last_op.state) + ' ' + last_op.date.strftime('%S%f'))

# [START app]
app = webapp2.WSGIApplication([
    ('/', MainPage),
    ('/do', DoOperation),
    ('/api', Api),
], debug=True)
# [END app]
