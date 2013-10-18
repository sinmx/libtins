/*
 * Copyright (c) 2012, Matias Fontanini
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 * 
 * * Redistributions of source code must retain the above copyright
 *   notice, this list of conditions and the following disclaimer.
 * * Redistributions in binary form must reproduce the above
 *   copyright notice, this list of conditions and the following disclaimer
 *   in the documentation and/or other materials provided with the
 *   distribution.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */
 
#ifndef TINS_PDU_H
#define TINS_PDU_H


#include <stdint.h>
#include <vector>
#include "macros.h"
#include "cxxstd.h"
#include "exceptions.h"

/** \brief The Tins namespace.
 */
namespace Tins {

    class PacketSender;
    class NetworkInterface;
    
    /**
     * The type used to store several PDU option values.
     */
    typedef std::vector<uint8_t> byte_array;

    /** \brief Base class for protocol data units.
     *
     * Every PDU implementation must inherit this one. PDUs can be serialized,
     * therefore allowing a PacketSender to send them through the corresponding
     * sockets. PDUs are created upwards: upper layers will be children of the
     * lower ones. Each PDU must provide its flag identifier. This will be most
     * likely added to its parent's data, hence it should be a valid identifier.
     * For example, IP should provide IPPROTO_IP.
     */
    class PDU {
    public:
        /**
         * The type that will be returned when serializing PDUs.
         */
        typedef byte_array serialization_type;

        /**
         * \brief Enum which identifies each type of PDU.
         *
         * This enum is used to identify the PDU type.
         */
        enum PDUType {
            RAW,
            ETHERNET_II,
            IEEE802_3,
            RADIOTAP,
            DOT11,
            DOT11_ACK,
            DOT11_ASSOC_REQ,
            DOT11_ASSOC_RESP,
            DOT11_AUTH,
            DOT11_BEACON,
            DOT11_BLOCK_ACK,
            DOT11_BLOCK_ACK_REQ,
            DOT11_CF_END,
            DOT11_DATA,
            DOT11_CONTROL,
            DOT11_DEAUTH,
            DOT11_DIASSOC,
            DOT11_END_CF_ACK,
            DOT11_MANAGEMENT,
            DOT11_PROBE_REQ,
            DOT11_PROBE_RESP,
            DOT11_PS_POLL,
            DOT11_REASSOC_REQ,
            DOT11_REASSOC_RESP,
            DOT11_RTS,
            DOT11_QOS_DATA,
            LLC,
            SNAP,
            IP,
            ARP,
            TCP,
            UDP,
            ICMP,
            BOOTP,
            DHCP,
            EAPOL,
            RC4EAPOL,
            RSNEAPOL,
            DNS,
            LOOPBACK,
            IPv6,
            ICMPv6,
            SLL,
            DHCPv6,
            DOT1Q,
            PPPOE,
            STP,
            PPI,
            USER_DEFINED_PDU = 1000
        };

        /** 
         * \brief Default constructor.
         */
        PDU();
        
        #if TINS_IS_CXX11
            /**
             * \brief Move constructor.
             * 
             * \param rhs The PDU to be moved.
             */
            PDU(PDU &&rhs) noexcept 
            : _inner_pdu(0)
            {
                std::swap(_inner_pdu, rhs._inner_pdu);
            }
            
            /**
             * \brief Move assignment operator.
             * 
             * \param rhs The PDU to be moved.
             */
            PDU& operator=(PDU &&rhs) noexcept {
                std::swap(_inner_pdu, rhs._inner_pdu);
                return *this;
            }
        #endif

        /** 
         * \brief PDU destructor.
         *
         * Deletes the inner pdu, as a consequence every child pdu is
         * deleted.
         */
        virtual ~PDU();

        /** \brief The header's size
         */
        virtual uint32_t header_size() const = 0;

        /** \brief Trailer's size.
         *
         * Some protocols require a trailer(like Ethernet). This defaults to 0.
         */
        virtual uint32_t trailer_size() const { return 0; }

        /** \brief The whole chain of PDU's size, including this one.
         *
         * Returns the sum of this and all children PDUs' size.
         */
        uint32_t size() const;

        /**
         * \brief Getter for the inner PDU.
         * \return The current inner PDU. Might be 0.
         */
        PDU *inner_pdu() const { return _inner_pdu; }
        
        /**
         * \brief Releases the inner PDU.
         * 
         * This method makes this PDU to <b>no longer own</b> the inner
         * PDU. The current inner PDU is returned, and is <b>not</b>
         * destroyed. That means after calling this function, you are 
         * responsible for using operator delete on the returned pointer.
         * 
         * Use this method if you want to somehow re-use a PDU that
         * is already owned by another PDU.
         * 
         * \return The current inner PDU. Might be 0.
         */
        PDU *release_inner_pdu();

        /**
         * \brief Sets the child PDU.
         *
         * When setting a new inner_pdu, the instance takesownership of
         * the object, therefore deleting it when it's no longer required.
         * 
         * \param next_pdu The new child PDU.
         */
        void inner_pdu(PDU *next_pdu);
        
        /**
         * \brief Sets the child PDU.
         *
         * The PDU parameter is cloned using PDU::clone.
         * 
         * \param next_pdu The new child PDU.
         */
        void inner_pdu(const PDU &next_pdu);


        /** 
         * \brief Serializes the whole chain of PDU's, including this one.
         *
         * This allocates a std::vector of size size(), and fills it
         * with the serialization this PDU, and all of the inner ones'.
         * 
         * \return serialization_type containing the serialization
         * of the whole stack of PDUs.
         */
        serialization_type serialize();

        /**
         * \brief Finds and returns the first PDU that matches the given flag.
         *
         * This method searches for the first PDU which has the same type flag as
         * the given one. If the first PDU matches that flag, it is returned.
         * If no PDU matches, 0 is returned.
         * \param flag The flag which being searched.
         */
        template<typename T> 
        T *find_pdu(PDUType type = T::pdu_flag) {
            PDU *pdu = this;
            while(pdu) {
                if(pdu->matches_flag(type))
                    return static_cast<T*>(pdu);
                pdu = pdu->inner_pdu();
            }
            return 0;
        }
        
        /**
         * \brief Finds and returns the first PDU that matches the given flag.
         *
         * \param flag The flag which being searched.
         */
        template<typename T> 
        const T *find_pdu(PDUType type = T::pdu_flag) const {
            return const_cast<PDU*>(this)->find_pdu<T>();
        }

        /**
         * \brief Finds and returns the first PDU that matches the given flag.
         * 
         * If the PDU is not found, a pdu_not_found exception is thrown.
         * 
         * \sa PDU::find_pdu
         * 
         * \param flag The flag which being searched.
         */
        template<typename T>
        T &rfind_pdu(PDUType type = T::pdu_flag) {
            T *ptr = find_pdu<T>(type);
            if(!ptr)
                throw pdu_not_found();
            return *ptr;
        }

        /**
         * \brief Finds and returns the first PDU that matches the given flag.
         *
         * \param flag The flag which being searched.
         */
        template<typename T> 
        const T &rfind_pdu(PDUType type = T::pdu_flag) const {
            return const_cast<PDU*>(this)->rfind_pdu<T>();
        }

        /**
         * \brief Clones this packet.
         *
         * This method clones this PDU and clones every inner PDU,
         * therefore obtaining a clone of the whole inner PDU chain.
         * The pointer returned must be deleted by the user.
         * \return A pointer to a clone of this packet.
         */
        virtual PDU *clone() const = 0;

        /** 
         * \brief Send the stack of PDUs through a PacketSender.
         *
         * This method will be called only for the PDU on the bottom of the stack,
         * therefore it should only implement this method if it can be sent.
         * 
         * PacketSender implements specific methods to send packets which start
         * on every valid TCP/IP stack layer; this should only be a proxy for
         * those methods.
         * 
         * If this PDU does not represent a link layer protocol, then
         * the interface argument will be ignored.
         * 
         * \param sender The PacketSender which will send the packet.
         * \param iface The network interface in which this packet will 
         * be sent.
         */
        virtual void send(PacketSender &sender, const NetworkInterface &iface);

        /** 
         * \brief Receives a matching response for this packet.
         *
         * This method should act as a proxy for PacketSender::recv_lX methods.
         * 
         * \param sender The packet sender which will receive the packet.
         * \param iface The interface in which to expect the response.
         */
        virtual PDU *recv_response(PacketSender &sender, const NetworkInterface &iface);

        /** 
         * \brief Check wether ptr points to a valid response for this PDU.
         *
         * This method must check wether the buffer pointed by ptr is a valid
         * response for this PDU. If it is valid, then it might want to propagate
         * the call to the next PDU. Note that in some cases, such as ICMP
         * Host Unreachable, there is no need to ask the next layer for matching.
         * \param ptr The pointer to the buffer.
         * \param total_sz The size of the buffer.
         */
        virtual bool matches_response(const uint8_t *ptr, uint32_t total_sz) const { 
            return false; 
        }

        /**
         * \brief Check wether this PDU matches the specified flag.
         *
         * This method should be reimplemented in PDU classes which have
         * subclasses, and try to match the given PDU to each of its parent
         * classes' flag.
         * \param flag The flag to match.
         */
        virtual bool matches_flag(PDUType flag) const {
           return flag == pdu_type();
        }

        /**
         * \brief Getter for the PDU's type.
         *
         * \return Returns the PDUType corresponding to the PDU.
         */
        virtual PDUType pdu_type() const = 0;
    protected:
        /**
         * \brief Copy constructor.
         */
        PDU(const PDU &other);

        /**
         * \brief Copy assignment operator.
         */
        PDU &operator=(const PDU &other);

        /**
         * \brief Copy other PDU's inner PDU(if any).
         * \param pdu The PDU from which to copy the inner PDU.
         */
        void copy_inner_pdu(const PDU &pdu);

        /**
         * \brief Prepares this PDU for serialization.
         * 
         * This method is called before the inner PDUs are serialized.
         * It's useful in situations such as when serializing IP PDUs,
         * which don't contain any link layer encapsulation, and therefore
         * require to set the source IP address before the TCP/UDP checksum
         * is calculated.
         * 
         * By default, this method does nothing
         * 
         * \param parent The parent PDU.
         */
        virtual void prepare_for_serialize(const PDU *parent) { }

        /** 
         * \brief Serializes this PDU and propagates this action to child PDUs.
         *
         * \param buffer The buffer in which to store this PDU's serialization.
         * \param total_sz The total size of the buffer.
         * \param parent The parent PDU. Will be 0 if there's the parent does not exist.
         */
        void serialize(uint8_t *buffer, uint32_t total_sz, const PDU *parent);

        /** 
         * \brief Serializes this TCP PDU.
         *
         * Each PDU must override this method and implement it's own
         * serialization.
         * \param buffer The buffer in which the PDU will be serialized.
         * \param total_sz The size available in the buffer.
         * \param parent The PDU that's one level below this one on the stack. Might be 0.
         */
        virtual void write_serialization(uint8_t *buffer, uint32_t total_sz, const PDU *parent) = 0;
    private:
        PDU *_inner_pdu;
    };
    
    /**
     * \brief Concatenation operator.
     * 
     * This operator concatenates several PDUs. A copy of the right 
     * operand is set at the end of the left one's inner PDU chain.
     * This means that:
     * 
     * IP some_ip = IP("127.0.0.1") / TCP(12, 13) / RawPDU("bleh");
     * 
     * Works as expected, meaning the output PDU will look like the 
     * following:
     * 
     * IP - TCP - RawPDU
     * 
     * \param lop The left operand, which will be the one modified.
     * \param rop The right operand, the one which will be appended
     * to lop.
     */
    template<typename T>
    T &operator/= (T &lop, const PDU &rop) {
        PDU *last = &lop;
        while(last->inner_pdu())
            last = last->inner_pdu();
        last->inner_pdu(rop.clone());
        return lop;
    }
    
    /**
     * \brief Concatenation operator.
     * 
     * \sa operator/=
     */
    template<typename T>
    T operator/ (T lop, const PDU &rop) {
        lop /= rop;
        return lop;
    }
    
    /**
     * \brief Concatenation operator on PDU pointers.
     * 
     * \sa operator/=
     */
    template<typename T>
    T *operator/= (T* lop, const PDU &rop) {
        *lop /= rop;
        return lop;
    }

    namespace Internals {
        template<typename T>
        struct remove_pointer {
            typedef T type;
        };

        template<typename T>
        struct remove_pointer<T*> {
            typedef T type;
        };
    }

    template<typename T, typename U>
    T tins_cast(U *pdu) {
        typedef typename Internals::remove_pointer<T>::type TrueT;
        return pdu && (TrueT::pdu_flag == pdu->pdu_type()) ?
            static_cast<T>(pdu) :
            0;
    }

    template<typename T, typename U>
    T &tins_cast(U &pdu) {
        T *ptr = tins_cast<T*>(&pdu);
        if(!ptr)
            throw bad_tins_cast();
        return *ptr;
    }
}

#endif // TINS_PDU_H
