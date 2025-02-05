boost::json::result_for<Action, boost::json::value>::type tag_invoke(
    boost::json::try_value_to_tag<Action>, const boost::json::value &jvRoot);

boost::json::result_for<Followers, boost::json::value>::type tag_invoke(
    boost::json::try_value_to_tag<Followers>, const boost::json::value &jvRoot);

boost::json::result_for<Slow, boost::json::value>::type tag_invoke(
    boost::json::try_value_to_tag<Slow>, const boost::json::value &jvRoot);

boost::json::result_for<Vip, boost::json::value>::type tag_invoke(
    boost::json::try_value_to_tag<Vip>, const boost::json::value &jvRoot);

boost::json::result_for<Unvip, boost::json::value>::type tag_invoke(
    boost::json::try_value_to_tag<Unvip>, const boost::json::value &jvRoot);

boost::json::result_for<Mod, boost::json::value>::type tag_invoke(
    boost::json::try_value_to_tag<Mod>, const boost::json::value &jvRoot);

boost::json::result_for<Unmod, boost::json::value>::type tag_invoke(
    boost::json::try_value_to_tag<Unmod>, const boost::json::value &jvRoot);

boost::json::result_for<Ban, boost::json::value>::type tag_invoke(
    boost::json::try_value_to_tag<Ban>, const boost::json::value &jvRoot);

boost::json::result_for<Unban, boost::json::value>::type tag_invoke(
    boost::json::try_value_to_tag<Unban>, const boost::json::value &jvRoot);

boost::json::result_for<Timeout, boost::json::value>::type tag_invoke(
    boost::json::try_value_to_tag<Timeout>, const boost::json::value &jvRoot);

boost::json::result_for<Untimeout, boost::json::value>::type tag_invoke(
    boost::json::try_value_to_tag<Untimeout>, const boost::json::value &jvRoot);

boost::json::result_for<Raid, boost::json::value>::type tag_invoke(
    boost::json::try_value_to_tag<Raid>, const boost::json::value &jvRoot);

boost::json::result_for<Unraid, boost::json::value>::type tag_invoke(
    boost::json::try_value_to_tag<Unraid>, const boost::json::value &jvRoot);

boost::json::result_for<Delete, boost::json::value>::type tag_invoke(
    boost::json::try_value_to_tag<Delete>, const boost::json::value &jvRoot);

boost::json::result_for<AutomodTerms, boost::json::value>::type tag_invoke(
    boost::json::try_value_to_tag<AutomodTerms>,
    const boost::json::value &jvRoot);

boost::json::result_for<UnbanRequest, boost::json::value>::type tag_invoke(
    boost::json::try_value_to_tag<UnbanRequest>,
    const boost::json::value &jvRoot);

boost::json::result_for<Warn, boost::json::value>::type tag_invoke(
    boost::json::try_value_to_tag<Warn>, const boost::json::value &jvRoot);

boost::json::result_for<Event, boost::json::value>::type tag_invoke(
    boost::json::try_value_to_tag<Event>, const boost::json::value &jvRoot);

boost::json::result_for<Payload, boost::json::value>::type tag_invoke(
    boost::json::try_value_to_tag<Payload>, const boost::json::value &jvRoot);
