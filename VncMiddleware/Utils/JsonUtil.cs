using Newtonsoft.Json;
using System.IO;
using System.Text;
using System;
using System.Runtime.Serialization.Json;

namespace VncMiddleware
{
    internal class JsonUtil
    {
        /// <summary>
        /// object to json
        /// </summary>
        /// <param name="obj">Objects to be converted</param>
        /// <returns>Converted string</returns>
        /// <exception cref="ArgumentNullException">Parameter is null</exception>
        public static string ObjectToJson<T>(T obj)
        {
            if (obj == null)
                throw new ArgumentNullException("Object argument value is null.");
            DataContractJsonSerializer serializer = new DataContractJsonSerializer(typeof(T));
            using (MemoryStream ms = new MemoryStream())
            {
                serializer.WriteObject(ms, obj);
                return Encoding.UTF8.GetString(ms.ToArray());
            }
        }

        /// <summary>
        /// Public json to object
        /// </summary>
        /// <param name="json">The json string to be converted</param>
        /// <returns>Converted Objects</returns>
        /// <exception cref="ArgumentNullException">Parameter is null</exception>
        public static T JsonToObject<T>(string json)
        {
            if (string.IsNullOrWhiteSpace(json))
            {
                throw new ArgumentNullException("Json argument value is null.");
            }
            return JsonConvert.DeserializeObject<T>(json);
        }
    }
}
