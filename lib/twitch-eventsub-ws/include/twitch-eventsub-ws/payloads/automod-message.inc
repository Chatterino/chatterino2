boost::json::result_for<Boundary, boost::json::value>::type tag_invoke(
    boost::json::try_value_to_tag<Boundary>, const boost::json::value &jvRoot);

boost::json::result_for<AutomodReason, boost::json::value>::type tag_invoke(
    boost::json::try_value_to_tag<AutomodReason>,
    const boost::json::value &jvRoot);

boost::json::result_for<FoundTerm, boost::json::value>::type tag_invoke(
    boost::json::try_value_to_tag<FoundTerm>, const boost::json::value &jvRoot);

boost::json::result_for<BlockedTermReason, boost::json::value>::type tag_invoke(
    boost::json::try_value_to_tag<BlockedTermReason>,
    const boost::json::value &jvRoot);
