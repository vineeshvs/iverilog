#ifndef __vvp_net_sig_H
#define __vvp_net_sig_H
/*
 * Copyright (c) 2004-2009 Stephen Williams (steve@icarus.com)
 *
 *    This source code is free software; you can redistribute it
 *    and/or modify it in source code form under the terms of the GNU
 *    General Public License as published by the Free Software
 *    Foundation; either version 2 of the License, or (at your option)
 *    any later version.
 *
 *    This program is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU General Public License for more details.
 *
 *    You should have received a copy of the GNU General Public License
 *    along with this program; if not, write to the Free Software
 *    Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA
 */

# include  "config.h"
# include  "vpi_user.h"
# include  "vvp_net.h"
# include  <stddef.h>
# include  <stdlib.h>
# include  <string.h>
# include  <new>
# include  <assert.h>

#ifdef HAVE_IOSFWD
# include  <iosfwd>
#else
class ostream;
#endif

using namespace std;

/*
 * Things derived from vvp_vpi_callback may have callbacks
 * attached. This is how vpi callbacks are attached to the vvp
 * structure.
 *
 * Things derived from vvp_vpi_callback may also be array'ed, so it
 * includes some members that arrays use.
 */
class vvp_vpi_callback {

    public:
      vvp_vpi_callback();
      virtual ~vvp_vpi_callback();

      void attach_as_word(class __vpiArray* arr, unsigned long addr);

      void add_vpi_callback(struct __vpiCallback*);
#ifdef CHECK_WITH_VALGRIND
	/* This has only been tested at EOS. */
      void clear_all_callbacks(void);
#endif

	// Derived classes implement this method to provide a way for
	// vpi to get at the vvp value of the object.
      virtual void get_value(struct t_vpi_value*value) =0;

    protected:
	// Derived classes call this method to indicate that it is
	// time to call the callback.
      void run_vpi_callbacks();

    private:
      struct __vpiCallback*vpi_callbacks_;
      class __vpiArray* array_;
      unsigned long array_word_;
};

/* vvp_fun_signal
 * This node is the place holder in a vvp network for signals,
 * including nets of various sort. The output from a signal follows
 * the type of its port-0 input. If vvp_vector4_t values come in
 * through port-0, then vvp_vector4_t values are propagated. If
 * vvp_vector8_t values come in through port-0, then vvp_vector8_t
 * values are propagated. Thus, this node is slightly polymorphic.
 *
 * If the signal is a net (i.e. a wire or tri) then this node will
 * have an input that is the data source. The data source will connect
 * through port-0.
 *
 * If the signal is a reg, then there will be no netlist input, the
 * values will be written by behavioral statements. The %set and
 * %assign statements will write through port-0.
 *
 * In any case, behavioral code is able to read the value that this
 * node last propagated, by using the value() method. That is important
 * functionality of this node.
 *
 * Continuous assignments are made through port-1. When a value is
 * written here, continuous assign mode is activated, and input
 * through port-0 is ignored until continuous assign mode is turned
 * off again. Writing into this port can be done in behavioral code
 * using the %cassign/v instruction, or can be done by the network by
 * hooking the output of a vvp_net_t to this port.
 *
 * Force assignments are made through port-2. When a value is written
 * here, force mode is activated. In force mode, port-0 data (or
 * port-1 data if in continuous assign mode) is tracked but not
 * propagated. The force value is propagated and is what is readable
 * through the value method.
 *
 * Port-3 is a command port, intended for use by procedural
 * instructions. The client must write long values to this port to
 * invoke the command of interest. The command values are:
 *
 *  1  -- deassign
 *          The deassign command takes the node out of continuous
 *          assignment mode. The output value is unchanged, and force
 *          mode, if active, remains in effect.
 *
 *  2  -- release/net
 *          The release/net command takes the node out of force mode,
 *          and propagates the tracked port-0 value to the signal
 *          output. This acts like a release of a net signal.
 *
 *  3  -- release/reg
 *          The release/reg command is similar to the release/net
 *          command, but the port-0 value is not propagated. Changes
 *          to port-0 (or port-1 if continuous assign is active) will
 *          propagate starting at the next input change.
 */


class vvp_filter_wire_base : public vvp_net_fil_t, public vvp_vpi_callback {

    public:
      vvp_filter_wire_base();
      ~vvp_filter_wire_base();

	// These are the virtual methods that we implement to perform
	// the wire-style filtering.
      const vvp_vector4_t* filter_vec4(const vvp_vector4_t&val);
      const vvp_vector8_t* filter_vec8(const vvp_vector8_t&val);
      bool filter_real(double&val);
      bool filter_long(long&val);

    public:
	// Force/release work in the filter by setting the forced
	// value using one of the force_* methods. This sets the
	// forced value as a mask of the bits of the vector that are
	// forced. The filter then automatically runs the filter on
	// the outputs that pass through. You can also get at the
	// filtering results using the filtered_* methods. The
	// release_mask() method releases bits of the vector.

	// Enable filter force.
      void force_vec4(const vvp_vector4_t&val, vvp_vector2_t mask);
      void force_vec8(const vvp_vector8_t&val, vvp_vector2_t mask);
      void force_real(double val, vvp_vector2_t mask);

	// Test the value against the filter.
      vvp_bit4_t filtered_value(const vvp_vector4_t&val, unsigned idx) const;
      vvp_scalar_t filtered_value(const vvp_vector8_t&val, unsigned idx) const;

      const vvp_vector4_t& filtered_vec4(const vvp_vector4_t&val) const;
      const vvp_vector8_t& filtered_vec8(const vvp_vector8_t&val) const;
      double filtered_real(double val) const;

	// Release the force on the bits set in the mask.
      void release_mask(vvp_vector2_t mask);

      virtual void release(vvp_net_ptr_t ptr, bool net) =0;
      virtual void release_pv(vvp_net_ptr_t ptr, bool net,
                              unsigned base, unsigned wid) =0;

    private:
	// Forced value
      vvp_vector2_t force_mask_;
      vvp_vector4_t force4_;
      vvp_vector8_t force8_;
      double force_real_;
	// True if the next filter must propagate. Need this to allow
	// the forced value to get through.
      bool force_propagate_;
	// This is used as a static return value.
      mutable vvp_vector4_t filter4_;
      mutable vvp_vector8_t filter8_;
};

class vvp_fun_signal_base : public vvp_net_fun_t, public vvp_filter_wire_base {

    public:
      vvp_fun_signal_base();

      void deassign();
      void deassign_pv(unsigned base, unsigned wid);

    public:

	/* The %force/link instruction needs a place to write the
	   source node of the force, so that subsequent %force and
	   %release instructions can undo the link as needed. */
      struct vvp_net_t*force_link;
      struct vvp_net_t*cassign_link;

    protected:
      bool continuous_assign_active_;
      vvp_vector2_t assign_mask_;

    protected:

	// This is true until at least one propagation happens.
      bool needs_init_;
};

/*
 * This abstract class is a little more specific than the signal_base
 * class, in that it adds vector access methods.
 */
class vvp_fun_signal_vec : public vvp_fun_signal_base {

    public:
	// For vector signal types, this returns the vector count.
      virtual unsigned size() const =0;
      virtual vvp_bit4_t value(unsigned idx) const =0;
      virtual vvp_scalar_t scalar_value(unsigned idx) const =0;
      virtual vvp_vector4_t vec4_value() const =0;
};

class vvp_fun_signal4 : public vvp_fun_signal_vec {

    public:
      explicit vvp_fun_signal4() {};

      void get_value(struct t_vpi_value*value);

};

/*
 * Statically allocated vvp_fun_signal4.
 */
class vvp_fun_signal4_sa : public vvp_fun_signal4 {

    public:
      explicit vvp_fun_signal4_sa(unsigned wid, vvp_bit4_t init=BIT4_X);

      void recv_vec4(vvp_net_ptr_t port, const vvp_vector4_t&bit,
                     vvp_context_t);
      void recv_vec8(vvp_net_ptr_t port, const vvp_vector8_t&bit);

	// Part select variants of above
      void recv_vec4_pv(vvp_net_ptr_t port, const vvp_vector4_t&bit,
			unsigned base, unsigned wid, unsigned vwid,
                        vvp_context_t);
      void recv_vec8_pv(vvp_net_ptr_t port, const vvp_vector8_t&bit,
			unsigned base, unsigned wid, unsigned vwid);

	// Get information about the vector value.
      unsigned   size() const;
      vvp_bit4_t value(unsigned idx) const;
      vvp_scalar_t scalar_value(unsigned idx) const;
      vvp_vector4_t vec4_value() const;

	// Commands
      void release(vvp_net_ptr_t port, bool net);
      void release_pv(vvp_net_ptr_t port, bool net,
                      unsigned base, unsigned wid);

    private:
      void calculate_output_(vvp_net_ptr_t ptr);

      vvp_vector4_t bits4_;
};

/*
 * Automatically allocated vvp_fun_signal4.
 */
class vvp_fun_signal4_aa : public vvp_fun_signal4, public automatic_hooks_s {

    public:
      explicit vvp_fun_signal4_aa(unsigned wid, vvp_bit4_t init=BIT4_X);

      void alloc_instance(vvp_context_t context);
      void reset_instance(vvp_context_t context);
#ifdef CHECK_WITH_VALGRIND
      void free_instance(vvp_context_t context);
#endif

      void recv_vec4(vvp_net_ptr_t port, const vvp_vector4_t&bit,
                     vvp_context_t context);

	// Part select variants of above
      void recv_vec4_pv(vvp_net_ptr_t port, const vvp_vector4_t&bit,
			unsigned base, unsigned wid, unsigned vwid,
                        vvp_context_t);

	// Get information about the vector value.
      unsigned   size() const;
      vvp_bit4_t value(unsigned idx) const;
      vvp_scalar_t scalar_value(unsigned idx) const;
      vvp_vector4_t vec4_value() const;

	// Commands
      void release(vvp_net_ptr_t port, bool net);
      void release_pv(vvp_net_ptr_t port, bool net,
                      unsigned base, unsigned wid);

    private:
      unsigned context_idx_;
      unsigned size_;
};

class vvp_fun_signal8  : public vvp_fun_signal_vec {

    public:
      explicit vvp_fun_signal8(unsigned wid);

      void recv_vec4(vvp_net_ptr_t port, const vvp_vector4_t&bit,
                     vvp_context_t context);
      void recv_vec8(vvp_net_ptr_t port, const vvp_vector8_t&bit);

	// Part select variants of above
      void recv_vec4_pv(vvp_net_ptr_t port, const vvp_vector4_t&bit,
                        unsigned base, unsigned wid, unsigned vwid,
                        vvp_context_t context);
      void recv_vec8_pv(vvp_net_ptr_t port, const vvp_vector8_t&bit,
                        unsigned base, unsigned wid, unsigned vwid);

	// Get information about the vector value.
      unsigned   size() const;
      vvp_bit4_t value(unsigned idx) const;
      vvp_scalar_t scalar_value(unsigned idx) const;
      vvp_vector4_t vec4_value() const;

	// Commands
      void release(vvp_net_ptr_t port, bool net);
      void release_pv(vvp_net_ptr_t port, bool net,
                      unsigned base, unsigned wid);

      void get_value(struct t_vpi_value*value);

    private:
      void calculate_output_(vvp_net_ptr_t ptr);

      vvp_vector8_t bits8_;
      vvp_vector8_t force_;
};

class vvp_fun_signal_real : public vvp_fun_signal_base {

    public:
      explicit vvp_fun_signal_real() {};

	// Get information about the vector value.
      virtual double real_value() const = 0;

      void get_value(struct t_vpi_value*value);
};

/*
 * Statically allocated vvp_fun_signal_real.
 */
class vvp_fun_signal_real_sa : public vvp_fun_signal_real {

    public:
      explicit vvp_fun_signal_real_sa();

      void recv_real(vvp_net_ptr_t port, double bit,
                     vvp_context_t);

	// Get information about the vector value.
      double real_value() const;

	// Commands
      void release(vvp_net_ptr_t port, bool net);
      void release_pv(vvp_net_ptr_t port, bool net,
                      unsigned base, unsigned wid);

    private:
      double bits_;
      double force_;
};

/*
 * Automatically allocated vvp_fun_signal_real.
 */
class vvp_fun_signal_real_aa : public vvp_fun_signal_real, public automatic_hooks_s {

    public:
      explicit vvp_fun_signal_real_aa();

      void alloc_instance(vvp_context_t context);
      void reset_instance(vvp_context_t context);
#ifdef CHECK_WITH_VALGRIND
      void free_instance(vvp_context_t context);
#endif

      void recv_real(vvp_net_ptr_t port, double bit,
                     vvp_context_t context);

	// Get information about the vector value.
      double real_value() const;

	// Commands
      void release(vvp_net_ptr_t port, bool net);
      void release_pv(vvp_net_ptr_t port, bool net,
                      unsigned base, unsigned wid);

    private:
      unsigned context_idx_;
};


#endif