#include <data_plane.h>
#include <iostream>

data_plane::data_plane(control_plane &control_plane) : _control_plane(control_plane) {}

// от абонента к apn
void data_plane::handle_uplink(uint32_t dp_teid, Packet &&packet) {
    std::shared_ptr<bearer> bearer = _control_plane.find_bearer_by_dp_teid(dp_teid);
    if (!bearer)
        return;

    if (!bearer->check_uplink_limit(packet.size())) {
        std::cerr << "Лимит превышен" << std::endl;
        return;
    }

    std::shared_ptr<pdn_connection> pdn = bearer->get_pdn_connection();
    forward_packet_to_apn(pdn->get_apn_gw(), std::move(packet));
};

// от сети к абоненту, используя sgw
void data_plane::handle_downlink(const boost::asio::ip::address_v4 &ue_ip, Packet &&packet) {
    std::shared_ptr<pdn_connection> pdn;
    if (!(pdn = _control_plane.find_pdn_by_ip_address(ue_ip)))
        return;

    std::shared_ptr<bearer> bearer = pdn->get_default_bearer();
    if (!bearer)
        return;

    if (!bearer->check_downlink_limit(packet.size())) {
        std::cerr << "Лимит превышен" << std::endl;
        return;
    }

    forward_packet_to_sgw(pdn->get_sgw_address(), bearer->get_sgw_dp_teid(), std::move(packet));
};