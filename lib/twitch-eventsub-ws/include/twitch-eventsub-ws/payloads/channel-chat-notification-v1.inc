boost::json::result_for<Badge, boost::json::value>::type tag_invoke(
    boost::json::try_value_to_tag<Badge>, const boost::json::value &jvRoot);

boost::json::result_for<Subcription, boost::json::value>::type tag_invoke(
    boost::json::try_value_to_tag<Subcription>,
    const boost::json::value &jvRoot);

boost::json::result_for<Resubscription, boost::json::value>::type tag_invoke(
    boost::json::try_value_to_tag<Resubscription>,
    const boost::json::value &jvRoot);

boost::json::result_for<GiftSubscription, boost::json::value>::type tag_invoke(
    boost::json::try_value_to_tag<GiftSubscription>,
    const boost::json::value &jvRoot);

boost::json::result_for<CommunityGiftSubscription, boost::json::value>::type
    tag_invoke(boost::json::try_value_to_tag<CommunityGiftSubscription>,
               const boost::json::value &jvRoot);

boost::json::result_for<GiftPaidUpgrade, boost::json::value>::type tag_invoke(
    boost::json::try_value_to_tag<GiftPaidUpgrade>,
    const boost::json::value &jvRoot);

boost::json::result_for<PrimePaidUpgrade, boost::json::value>::type tag_invoke(
    boost::json::try_value_to_tag<PrimePaidUpgrade>,
    const boost::json::value &jvRoot);

boost::json::result_for<Raid, boost::json::value>::type tag_invoke(
    boost::json::try_value_to_tag<Raid>, const boost::json::value &jvRoot);

boost::json::result_for<Unraid, boost::json::value>::type tag_invoke(
    boost::json::try_value_to_tag<Unraid>, const boost::json::value &jvRoot);

boost::json::result_for<PayItForward, boost::json::value>::type tag_invoke(
    boost::json::try_value_to_tag<PayItForward>,
    const boost::json::value &jvRoot);

boost::json::result_for<Announcement, boost::json::value>::type tag_invoke(
    boost::json::try_value_to_tag<Announcement>,
    const boost::json::value &jvRoot);

boost::json::result_for<CharityDonationAmount, boost::json::value>::type
    tag_invoke(boost::json::try_value_to_tag<CharityDonationAmount>,
               const boost::json::value &jvRoot);

boost::json::result_for<CharityDonation, boost::json::value>::type tag_invoke(
    boost::json::try_value_to_tag<CharityDonation>,
    const boost::json::value &jvRoot);

boost::json::result_for<BitsBadgeTier, boost::json::value>::type tag_invoke(
    boost::json::try_value_to_tag<BitsBadgeTier>,
    const boost::json::value &jvRoot);

boost::json::result_for<Event, boost::json::value>::type tag_invoke(
    boost::json::try_value_to_tag<Event>, const boost::json::value &jvRoot);

boost::json::result_for<Payload, boost::json::value>::type tag_invoke(
    boost::json::try_value_to_tag<Payload>, const boost::json::value &jvRoot);
