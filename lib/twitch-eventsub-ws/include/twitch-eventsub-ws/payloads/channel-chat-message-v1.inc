boost::json::result_for<Badge, boost::json::value>::type tag_invoke(
    boost::json::try_value_to_tag<Badge>, const boost::json::value &jvRoot);

boost::json::result_for<Cheer, boost::json::value>::type tag_invoke(
    boost::json::try_value_to_tag<Cheer>, const boost::json::value &jvRoot);

boost::json::result_for<Reply, boost::json::value>::type tag_invoke(
    boost::json::try_value_to_tag<Reply>, const boost::json::value &jvRoot);

boost::json::result_for<Event, boost::json::value>::type tag_invoke(
    boost::json::try_value_to_tag<Event>, const boost::json::value &jvRoot);

boost::json::result_for<Payload, boost::json::value>::type tag_invoke(
    boost::json::try_value_to_tag<Payload>, const boost::json::value &jvRoot);
