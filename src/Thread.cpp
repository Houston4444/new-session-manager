
/*******************************************************************************/
/* Copyright (C) 2008-2020 Jonathan Moore Liles (as "Non-Session-Manager")     */
/* Copyright (C) 2020- Nils Hilbricht                                          */
/*                                                                             */
/* This file is part of New-Session-Manager                                    */
/*                                                                             */
/* New-Session-Manager is free software: you can redistribute it and/or modify */
/* it under the terms of the GNU General Public License as published by        */
/* the Free Software Foundation, either version 3 of the License, or           */
/* (at your option) any later version.                                         */
/*                                                                             */
/* New-Session-Manager is distributed in the hope that it will be useful,      */
/* but WITHOUT ANY WARRANTY; without even the implied warranty of              */
/* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the               */
/* GNU General Public License for more details.                                */
/*                                                                             */
/* You should have received a copy of the GNU General Public License           */
/* along with New-Session-Manager. If not, see <https://www.gnu.org/licenses/>.*/
/*******************************************************************************/

#include "Thread.hpp"
#include <assert.h>
#include <string.h>



pthread_key_t Thread::_current = 0;



Thread::Thread ( )
{
    _thread = 0;
    _running = false;
    _name = 0;
}

Thread::Thread ( const char *name )
{
    _thread = 0;
    _running = false;
    _name = name;
}

void
Thread::init ( void )
{
    pthread_key_create( &_current, NULL );
}

bool
Thread::is ( const char *name )
{
    return ! strcmp( Thread::current()->name(), name );
}

/** to be used by existing threads (that won't call clone()) */
void
Thread::set ( const char *name )
{
    _thread = pthread_self();
    _name = name;
    _running = true;

    pthread_setspecific( _current, (void*)this );
}

Thread *
Thread::current ( void )
{
    return (Thread*)pthread_getspecific( _current );
}


struct thread_data
{
    void *(*entry_point)(void *);
    void *arg;
    void *t;
};

void *
Thread::run_thread ( void *arg )
{
    thread_data td = *(thread_data *)arg;
    delete (thread_data*)arg;

    pthread_setspecific( _current, td.t );

    ((Thread*)td.t)->_running = true;

    void * r = td.entry_point( td.arg );

    ((Thread*)td.t)->_running = false;

    return r;
}


bool
Thread::clone ( void *(*entry_point)(void *), void *arg )
{
    assert( ! _thread );

    thread_data *td = new thread_data;
    td->entry_point = entry_point;
    td->arg = arg;
    td->t = this;

    if ( pthread_create( &_thread, NULL, run_thread, td ) != 0 )
        return false;

    return true;
}

void
Thread::detach ( void )
{
    pthread_detach( _thread );
    _thread = 0;
}

void
Thread::cancel ( void )
{
    pthread_cancel( _thread );
    _thread = 0;
}

void
Thread::join ( void )
{
    if ( _thread != 0 )
    {
        /* not joined yet, go ahead. */
        pthread_join( _thread, NULL );
    }
    _thread = 0;
}

/* never call this unless some other thread will be calling 'join' on
 * this one, otherwise, running() will return true even though the
 * thread is dead */
void
Thread::exit ( void *retval )
{
    _running = false;
    pthread_exit( retval );
}
