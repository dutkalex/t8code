/*
This file is part of t8code.
t8code is a C library to manage a collection (a forest) of multiple
connected adaptive space-trees of general element classes in parallel.

Copyright (C) 2024 the developers

t8code is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

t8code is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with t8code; if not, write to the Free Software Foundation, Inc.,
51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
*/

#ifndef T8_DATA_HANDLER_HXX
#define T8_DATA_HANDLER_HXX

#include <t8.h>
#include <vector>
#include <t8_data/t8_data_handler_base.hxx>
#include <algorithm>
#include <memory>
#include <type_traits>

class t8_abstract_data_handler {
 public:
  /**
   * Pure virtual function to determine the buffer size.
   *
   * This function must be implemented by derived classes to calculate
   * the size of the buffer required for communication.
   *
   * \param[in] comm The MPI communicator.
   * \return The size of the buffer.
   */
  virtual int
  buffer_size (sc_MPI_Comm comm)
    = 0;

  /**
   * Packs a vector into a buffer. The vector data will be prefixed with the number of elements in the vector.
   *
   * This pure virtual function is responsible for packing a vector prefix into the provided buffer.
   *
   * \param[in, out] buffer A pointer to the buffer where the vector prefix will be packed.
   * \param[in] num_bytes The number of bytes to be packed.
   * \param[in] pos A reference to an integer representing the current position in the buffer. This will be updated as bytes are packed.
   * \param[in] comm The MPI communicator used for the operation.
   */
  virtual void
  pack_vector_prefix (void *buffer, const int num_bytes, int &pos, sc_MPI_Comm comm)
    = 0;

  /**
   * Unpacks a vector from a buffer. Expected to be prefixed with the number of elements in the vector.
   *
   * This pure virtual function is responsible for unpacking a vector prefix from the provided buffer.
   *
   * \param[in] buffer Pointer to the buffer containing the packed data.
   * \param[in] num_bytes The number of bytes in the buffer.
   * \param[in] pos Reference to an integer representing the current position in the buffer. This will be updated as data is unpacked.
   * \param[in] outcount Reference to an integer where the count of unpacked elements will be stored.
   * \param[in] comm The MPI communicator used for the operation.
   */
  virtual void
  unpack_vector_prefix (const void *buffer, const int num_bytes, int &pos, int &outcount, sc_MPI_Comm comm)
    = 0;

  /**
   * Pure virtual function to send data to a specified destination.
   *
   * This function is responsible for packing and sending data to a given destination
   * with a specific tag using the provided MPI communicator.
   *
   * \param[in] dest The destination rank to which the data will be sent.
   * \param[in] tag The tag associated with the message to be sent.
   * \param[in] comm The MPI communicator used for the communication.
   * \return An integer indicating the status of the send operation.
   */
  virtual int
  send (const int dest, const int tag, sc_MPI_Comm comm)
    = 0;

  /**
   * Receives a message from a specified source.
   *
   * This pure virtual function is responsible for receiving and unpacking a message from a given source
   * with a specific tag within the provided MPI communicator. The function will also
   * update the status and output count of the received message.
   *
   * \param[in] source The rank of the source process from which the message is received.
   * \param[in] tag The tag of the message to be received.
   * \param[in] comm The MPI communicator within which the message is received.
   * \param[in] status A pointer to an MPI status object that will be updated with the status of the received message.
   * \param[in] outcount A reference to an integer that will be updated with the count of received elements.
   * \return An integer indicating the success or failure of the receive operation.
   */
  virtual int
  recv (const int source, const int tag, sc_MPI_Comm comm, sc_MPI_Status *status, int &outcount)
    = 0;

  /**
   * Pure virtual function to get the type.
   * 
   * This function must be overridden in derived classes to return the type.
   * 
   * \return An integer representing the type.
   */
  virtual int
  type ()
    = 0;

  virtual ~t8_abstract_data_handler () {};
};

/**
 * \class t8_data_handler
 * \brief A template class for handling data in a distributed environment.
 *
 * This class inherits from t8_abstract_data_handler and provides methods for
 * packing, unpacking, sending, and receiving data using MPI.
 *
 * \tparam T The type of data to be handled.
 */
template <typename T>
class t8_data_handler: public t8_abstract_data_handler {
 public:
  t8_data_handler (): single_handler ()
  {
    m_data = nullptr;
  }

  t8_data_handler (const std::vector<T> &data): m_data (std::make_shared<std::vector<T>> (data)), single_handler ()
  {
  }

  void
  get_data (std::vector<T> &data) const
  {
    if (m_data) {
      data = *m_data;
    }
  }

  int
  buffer_size (sc_MPI_Comm comm) override
  {
    int total_size = 0;
    const int mpiret = sc_MPI_Pack_size (1, sc_MPI_INT, comm, &total_size);
    SC_CHECK_MPI (mpiret);
    if (m_data) {
      for (const auto &item : *m_data) {
        total_size += single_handler.size (item, comm);
      }
    }
    return total_size;
  }

  void
  pack_vector_prefix (void *buffer, const int num_bytes, int &pos, sc_MPI_Comm comm) override
  {
    const int num_data = m_data->size ();
    const int mpiret = sc_MPI_Pack (&num_data, 1, sc_MPI_INT, buffer, num_bytes, &pos, comm);
    SC_CHECK_MPI (mpiret);

    for (const auto &item : *m_data) {
      single_handler.pack (item, pos, buffer, num_bytes, comm);
    }
  }

  void
  unpack_vector_prefix (const void *buffer, const int num_bytes, int &pos, int &outcount, sc_MPI_Comm comm) override
  {
    const int mpiret = sc_MPI_Unpack (buffer, num_bytes, &pos, &outcount, 1, sc_MPI_INT, comm);
    SC_CHECK_MPI (mpiret);
    T8_ASSERT (outcount >= 0);

    if (!m_data) {
      m_data = std::make_shared<std::vector<T>> (outcount);
    }
    else {
      m_data->resize (outcount);
    }
    for (auto &item : *m_data) {
      single_handler.unpack (buffer, num_bytes, pos, item, comm);
    }
  }

  int
  send (const int dest, const int tag, sc_MPI_Comm comm) override
  {
#if T8_ENABLE_MPI
    int pos = 0;
    const int num_bytes = buffer_size (comm);
    std::vector<char> buffer (num_bytes);
    pack_vector_prefix (buffer.data (), num_bytes, pos, comm);

    const int mpiret = sc_MPI_Send (buffer.data (), num_bytes, sc_MPI_PACKED, dest, tag, comm);
    SC_CHECK_MPI (mpiret);
    return mpiret;
#else
    t8_infof ("send only available when configured with --enable-mpi\n");
    return sc_MPI_ERR_OTHER;
#endif
  }

  int
  recv (const int source, const int tag, sc_MPI_Comm comm, sc_MPI_Status *status, int &outcount) override
  {
#if T8_ENABLE_MPI
    int pos = 0;
    int mpiret = sc_MPI_Probe (source, tag, comm, status);
    SC_CHECK_MPI (mpiret);

    int num_bytes;
    mpiret = sc_MPI_Get_count (status, sc_MPI_PACKED, &num_bytes);
    SC_CHECK_MPI (mpiret);
    std::vector<char> buffer (num_bytes);

    mpiret = sc_MPI_Recv (buffer.data (), num_bytes, sc_MPI_PACKED, source, tag, comm, status);
    SC_CHECK_MPI (mpiret);
    unpack_vector_prefix (buffer.data (), num_bytes, pos, outcount, comm);
    return mpiret;
#else
    t8_infof ("recv only available when configured with --enable-mpi\n");
    return sc_MPI_ERR_OTHER;
#endif
  }

  int
  type () override
  {
    return single_handler.type ();
  }

 private:
  std::shared_ptr<std::vector<T>> m_data;
  t8_single_data_handler<T> single_handler;
};

#endif /* T8_DATA_HANDLER_HXX */
