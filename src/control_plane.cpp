#include <control_plane.h>
#include <iostream>
#include <random>
#include "pdn_connection.h"

#include "bearer.h"

using std::cout;
using std::endl;

// для реализации этого м-да в мапе _pdns
// ищем по teid указатель на pdn и return`им его
std::shared_ptr<pdn_connection> control_plane::find_pdn_by_cp_teid(uint32_t cp_teid) const {
    const auto search = _pdns.find(cp_teid);

    if (search == _pdns.end()) {
        return nullptr;
    }

    return search->second;
}

// тоже самое, только мэп другой
std::shared_ptr<pdn_connection> control_plane::find_pdn_by_ip_address(const boost::asio::ip::address_v4 &ip) const {
    const auto search = _pdns_by_ue_ip_addr.find(ip);

    if (search == _pdns_by_ue_ip_addr.end()) {
        return nullptr;
    }

    return search->second;
}

std::shared_ptr<bearer> control_plane::find_bearer_by_dp_teid(uint32_t dp_teid) const {
    const auto search = _bearers.find(dp_teid);

    if (search == _bearers.end())
        return nullptr;

    return search->second;
}

// удаление конекта через erase
void control_plane::delete_pdn_connection(uint32_t cp_teid) {
    if (_pdns.erase(cp_teid) == 0) {
        cout << "pdn не найден по teid\n";
        return;
    }

    cout << "pdn connection успешно удален " << cp_teid << endl;
}

// добавление apn в маппу
void control_plane::add_apn(std::string apn_name, boost::asio::ip::address_v4 apn_gateway) {
    auto addor = _apns.emplace(apn_name, apn_gateway);

    if (!addor.second)
        throw std::runtime_error("apn существует");
}

// создание pdn подключения
// генерация случайного ip
// поиск в мапе apn`а
// дальше через метод create взаимод. с конструктором и
// создание pdn connection
// после pdn добавляется в мапы
std::shared_ptr<pdn_connection> control_plane::create_pdn_connection(const std::string &apn,
    boost::asio::ip::address_v4 sgw_addr, uint32_t sgw_cp_teid) {

    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dist(0, 255);
    std::string ip_addr_gen = std::to_string(dist(gen)) + "." + std::to_string(dist(gen))
        + "." + std::to_string(dist(gen)) + "." + std::to_string(dist(gen));
    auto ue_ip = boost::asio::ip::make_address_v4(ip_addr_gen);

    auto apn_search = _apns.find(apn);
    if (apn_search == _apns.end()) {
        cout << "apn не найден " << apn << endl;
        return nullptr;
    }

    std::shared_ptr<pdn_connection> connection_pdn = pdn_connection::create(sgw_cp_teid, apn_search->second, ue_ip);
    connection_pdn->set_sgw_addr(sgw_addr);

    _pdns[sgw_cp_teid] = connection_pdn;
    _pdns_by_ue_ip_addr[ue_ip] = connection_pdn;

    return connection_pdn;
}

// сздание бирера и установление дефолтным бирером
std::shared_ptr<bearer> control_plane::create_bearer(const std::shared_ptr<pdn_connection> &pdn, uint32_t sgw_teid) {
    std::shared_ptr<bearer> c_bearer = std::make_shared<bearer>(sgw_teid, *pdn);
    c_bearer->set_sgw_dp_teid(sgw_teid);

    _bearers[sgw_teid] = c_bearer;

    if (!pdn->get_default_bearer()) {
        pdn->set_default_bearer(c_bearer);
    }

    return c_bearer;
}

// реализация м-да удаления бирера
void control_plane::delete_bearer(uint32_t dp_teid) {
    if (_bearers.erase(dp_teid) == 0) {
        cout << "бирер не найден по teid\n";
        return;
    }

    cout << "бирер успешно удален " << dp_teid << endl;
}

void control_plane::reset_all_limits() {
    for (const auto& [id, bearer] : _bearers) {
        bearer->reset_counters();
    }
}