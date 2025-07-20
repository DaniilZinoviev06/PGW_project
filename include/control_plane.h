// комментарии больше для себя писал, для удобства
#pragma once

#include <pdn_connection.h>
#include <boost/asio/ip/address.hpp>
#include <memory>
#include <boost/asio.hpp>

class control_plane {
public:
    std::shared_ptr<pdn_connection> find_pdn_by_cp_teid(uint32_t cp_teid) const; // мд для поиска pdn по teid

    std::shared_ptr<pdn_connection> find_pdn_by_ip_address(const boost::asio::ip::address_v4 &ip) const; // мд для поиска pdn по ip

    std::shared_ptr<bearer> find_bearer_by_dp_teid(uint32_t dp_teid) const; // мд для поиска бирерв по teid

    std::shared_ptr<pdn_connection> create_pdn_connection(const std::string &apn,
        boost::asio::ip::address_v4 sgw_addr, uint32_t sgw_cp_teid); // мд создание pdn соед.

    void delete_pdn_connection(uint32_t cp_teid); // мд удаление pdn connection

    std::shared_ptr<bearer> create_bearer(const std::shared_ptr<pdn_connection> &pdn, uint32_t sgw_teid);

    void delete_bearer(uint32_t dp_teid);

    void add_apn(std::string apn_name, boost::asio::ip::address_v4 apn_gateway);

    void reset_all_limits();

private:
    std::unordered_map<uint32_t, std::shared_ptr<pdn_connection>> _pdns; // маппа с pdn соед.
    std::unordered_map<boost::asio::ip::address_v4, std::shared_ptr<pdn_connection>> _pdns_by_ue_ip_addr; // маппа с pdn соед. по ip юзера
    std::unordered_map<uint32_t, std::shared_ptr<bearer>> _bearers; // маппа с бирерами
    std::unordered_map<std::string, boost::asio::ip::address_v4> _apns; // маппа с апнами

};
